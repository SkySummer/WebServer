#include "server.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <format>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int MAX_EVENTS = 1024;  // epoll 支持的最大事件数

inline sockaddr* toSockaddr(sockaddr_in* addr) {
    return reinterpret_cast<sockaddr*>(addr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

Server::Server(const uint16_t port, const size_t thread_count, Logger& logger)
    : port_(port), thread_pool_(thread_count, logger), logger_(logger) {
    setupSocket();
    setupEpoll();
}

Server::~Server() {
    close(listen_fd_);
    close(epoll_fd_);
    logger_.log(LogLevel::INFO, "Server resources cleaned up and shutting down.");
    logger_.logDivider("Server close");
}

void Server::setupSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        logger_.log(LogLevel::ERROR, "Failed to create socket.");
        throw std::runtime_error("Failed to create socket.");
    }

    // 配置服务器地址结构
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 设置 socket 选项：快速重用地址
    constexpr int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定 socket 到地址
    if (bind(listen_fd_, toSockaddr(&addr), sizeof(addr)) == -1) {
        logger_.log(LogLevel::ERROR, "Failed to bind socket.");
        throw std::runtime_error("Failed to bind socket.");
    }

    // 开始监听连接请求
    if (listen(listen_fd_, SOMAXCONN) == -1) {
        logger_.log(LogLevel::ERROR, "Failed to listen on socket.");
        throw std::runtime_error("Failed to listen on socket.");
    }

    // 设置监听 socket 为非阻塞
    setNonBlocking(listen_fd_);

    logger_.log(LogLevel::INFO, std::format("Listening on port {}", port_));
}

void Server::setupEpoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        logger_.log(LogLevel::ERROR, "Failed to create epoll instance.");
        throw std::runtime_error("Failed to create epoll instance.");
    }

    // 添加监听 socket 到 epoll 监听列表
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event);

    logger_.log(LogLevel::INFO, "Epoll instance created and listening socket added.");
}

void Server::run() {
    logger_.logDivider("Server start");

    while (true) {
        std::array<epoll_event, MAX_EVENTS> events{};
        const int event_count = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < event_count; ++i) {
            if (const int client_fd = events.at(i).data.fd; client_fd == listen_fd_) {
                handleNewConnection();
            } else {
                dispatchClient(client_fd);
            }
        }
        processCloseList();
    }
}

void Server::processCloseList() {
    while (true) {
        int client_fd{};
        {
            std::lock_guard lock(close_mutex_);
            if (close_list_.empty()) {
                break;
            }
            client_fd = *close_list_.begin();
        }

        disconnectClient(client_fd);

        std::lock_guard lock(close_mutex_);
        close_list_.erase(client_fd);
    }
}

void Server::disconnectClient(const int client_fd) {
    Address info;
    {
        std::lock_guard lock(clients_mutex_);
        const auto clients_iter = clients_.find(client_fd);
        if (clients_iter == clients_.end()) {
            return;
        }
        info = clients_iter->second;
    }

    // 从 epoll 中移除 fd，防止后续再触发事件
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
        logger_.log(LogLevel::WARNING, info, std::format("epoll_ctl DEL failed: {}", std::strerror(errno)));
    }

    if (close(client_fd) == -1) {
        logger_.log(LogLevel::WARNING, info, std::format("close failed: {}", std::strerror(errno)));
    }

    {
        std::lock_guard lock(clients_mutex_);
        clients_.erase(client_fd);
    }

    logger_.log(LogLevel::INFO, info, "Client disconnected.");
}

void Server::requestCloseClient(const int client_fd) {
    const Address info = getClientInfo(client_fd);
    std::lock_guard lock(close_mutex_);

    if (close_list_.insert(client_fd).second) {
        logger_.log(LogLevel::DEBUG, info, "Added to close list.");
    } else {
        logger_.log(LogLevel::DEBUG, info, "Already in close list, ignoring duplicate.");
    }
}

void Server::handleNewConnection() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        const int client_fd = accept(listen_fd_, toSockaddr(&client_addr), &len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 无更多连接
            }
            logger_.log(LogLevel::ERROR, "Failed to accept client connection.");
            throw std::runtime_error("Failed to accept client connection.");
        }

        // 设置客户端 socket 为非阻塞
        setNonBlocking(client_fd);

        // 添加客户端 socket 到 epoll 中，监听读写事件
        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = client_fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event);

        const Address info(client_addr, client_fd);

        {
            std::lock_guard lock(clients_mutex_);
            clients_[client_fd] = info;
        }

        logger_.log(LogLevel::INFO, info, "New client connected.");
    }
}

void Server::handleClientData(const int client_fd) {
    {
        std::scoped_lock lock(clients_mutex_, close_mutex_);
        if (!clients_.contains(client_fd) || close_list_.contains(client_fd)) {
            return;
        }
    }

    constexpr std::size_t buffer_size = 4096;
    std::array<char, buffer_size> buffer{};  // 用于存储从客户端接收到的数据
    const ssize_t bytes_read = read(client_fd, buffer.data(), buffer.size());
    const Address info = getClientInfo(client_fd);

    if (bytes_read == 0) {
        // 如果读到 0 字节，说明客户端关闭连接
        requestCloseClient(client_fd);
        return;
    }

    if (bytes_read < 0) {
        if (errno == EBADF) {
            logger_.log(LogLevel::WARNING, info, "Read on invalid fd: already closed elsewhere.");
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            logger_.log(LogLevel::ERROR, info, std::format("Read error: {}", strerror(errno)));
            requestCloseClient(client_fd);
        }
        return;
    }

    // 将读取的内容转换为 std::string 以便处理
    std::string request(buffer.data(), bytes_read);
    std::string method;
    std::string path;

    // 提取 HTTP 请求方法和请求路径
    if (const size_t method_end = request.find(' '); method_end != std::string::npos) {
        method = request.substr(0, method_end);

        const size_t path_start = method_end + 1;
        if (const size_t path_end = request.find(' ', path_start); path_end != std::string::npos) {
            path = request.substr(path_start, path_end - path_start);
        }
    }

    // 根据路径构造不同的响应体内容
    std::string body;
    std::string status = "200 OK";
    std::string content_type = "text/plain; charset=UTF-8";

    // 根据方法和路径进行不同的处理
    if (method == "GET") {
        logger_.log(LogLevel::DEBUG, info, std::format("Handling GET for path: {}", path));
        body = static_file_.serve(path, info, status, content_type);
    } else if (method == "POST") {
        logger_.log(LogLevel::DEBUG, info, std::format("Handling POST for path: {}", path));
        body = handlePOST(path, request);
    } else {
        logger_.log(LogLevel::WARNING, info, std::format("Unsupported method: {} on path: {}", method, path));
        constexpr int error_code = 405;
        body = StaticFile::respondWithError(error_code, status, content_type);
    }

    // 发送固定的 HTTP 响应（状态行 + 头部 + 空行 + 内容）
    const std::string response = std::format(
        "HTTP/1.1 {}\r\n"
        "Content-Type: {}\r\n"
        "Content-Length: {}\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{}",
        status, content_type, body.size(), body);

    write(client_fd, response.c_str(), response.size());
}

std::string Server::handlePOST(const std::string& path, const std::string& request) const {
    size_t body_start = request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        return "400 Bad Request";
    }

    body_start += 4;  // 跳过空行部分
    const std::string body = request.substr(body_start);

    logger_.log(LogLevel::DEBUG, std::format("POST body: {}", body));

    if (path == "/data") {
        return "Received POST data: " + body;
    }
    return "404 Not Found";
}

void Server::dispatchClient(const int client_fd) {
    try {
        thread_pool_.enqueue([this, client_fd] { handleClientData(client_fd); });
    } catch (const std::exception& e) {
        logger_.log(LogLevel::ERROR, getClientInfo(client_fd), std::format("Failed to enqueue task: {}", e.what()));
        requestCloseClient(client_fd);
    }
}

const Address& Server::getClientInfo(const int client_fd) {
    static const Address unknown;

    std::lock_guard lock(clients_mutex_);

    if (clients_.contains(client_fd)) {
        return clients_[client_fd];
    }
    return unknown;
}

int Server::setNonBlocking(const int socket_fd) {
    const int old_flags = fcntl(socket_fd, F_GETFL);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    return fcntl(socket_fd, F_SETFL, old_flags | O_NONBLOCK);  // NOLINT(cppcoreguidelines-pro-type-vararg)
}
