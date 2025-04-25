#include "core/server.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <format>
#include <mutex>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/address.h"
#include "core/http_response.h"
#include "utils/form_parser.h"
#include "utils/logger.h"

constexpr int MAX_EVENTS = 1024;  // epoll 支持的最大事件数

inline sockaddr* toSockaddr(sockaddr_in* addr) {
    return reinterpret_cast<sockaddr*>(addr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

Server::Server(const uint16_t port, const bool linger, const size_t thread_count, Logger* logger)
    : port_(port), linger_(linger), thread_pool_(thread_count, logger), logger_(logger) {
    setupSocket();
    setupEpoll();
}

Server::~Server() {
    close(listen_fd_);
    close(epoll_fd_);
    close(event_fd_);
    logger_->log(LogLevel::INFO, "Server resources cleaned up and shutting down.");
    logger_->logDivider("Server close");
}

void Server::setupSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        logger_->log(LogLevel::ERROR, "Failed to create socket.");
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
        logger_->log(LogLevel::ERROR, "Failed to bind socket.");
        throw std::runtime_error("Failed to bind socket.");
    }

    // 开始监听连接请求
    if (listen(listen_fd_, SOMAXCONN) == -1) {
        logger_->log(LogLevel::ERROR, "Failed to listen on socket.");
        throw std::runtime_error("Failed to listen on socket.");
    }

    // 设置监听 socket 为非阻塞
    setNonBlocking(listen_fd_);

    logger_->log(LogLevel::INFO, std::format("Listening on port {}", port_));
}

void Server::setupEpoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        logger_->log(LogLevel::ERROR, "Failed to create epoll instance.");
        throw std::runtime_error("Failed to create epoll instance.");
    }

    // 添加监听 socket 到 epoll 监听列表
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event);

    // 创建并注册 eventfd 用于唤醒 epoll_wait
    event_fd_ = eventfd(0, EFD_NONBLOCK);
    if (event_fd_ == -1) {
        logger_->log(LogLevel::ERROR, "Failed to create eventfd.");
        throw std::runtime_error("Failed to create eventfd.");
    }

    epoll_event wakeup_event{};
    wakeup_event.events = EPOLLIN;
    wakeup_event.data.fd = event_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd_, &wakeup_event);

    logger_->log(LogLevel::INFO, "Epoll instance created and listening socket added.");
}

void Server::run() {
    logger_->logDivider("Server start");

    std::array<epoll_event, MAX_EVENTS> events{};
    while (true) {
        const int event_count = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < event_count; ++i) {
            if (const int client_fd = events.at(i).data.fd; client_fd == listen_fd_) {
                handleNewConnection();
            } else if (client_fd == event_fd_) {
                // 读取 eventfd 数据，清空状态
                uint64_t dummy{};
                read(event_fd_, &dummy, sizeof(dummy));
                processCloseList();
            } else {
                dispatchClient(client_fd);
            }
        }
    }
}

void Server::processCloseList() {
    close_list_dirty_.clear();

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
        logger_->log(LogLevel::WARNING, info, std::format("epoll_ctl DEL failed: {}", std::strerror(errno)));
    }

    if (close(client_fd) == -1) {
        logger_->log(LogLevel::WARNING, info, std::format("close failed: {}", std::strerror(errno)));
    }

    {
        std::lock_guard lock(clients_mutex_);
        clients_.erase(client_fd);
    }

    logger_->log(LogLevel::INFO, info, "Client disconnected.");
}

void Server::requestCloseClient(const int client_fd) {
    const Address info = getClientInfo(client_fd);
    bool inserted = false;

    {
        std::lock_guard lock(close_mutex_);
        inserted = close_list_.insert(client_fd).second;
    }

    if (inserted) {
        logger_->log(LogLevel::DEBUG, info, "Added to close list.");

        if (!close_list_dirty_.test_and_set()) {
            constexpr uint64_t notify = 1;
            if (write(event_fd_, &notify, sizeof(notify)) != sizeof(notify)) {
                logger_->log(LogLevel::WARNING, info, "Failed to wake epoll via eventfd.");
            }
        }
    } else {
        logger_->log(LogLevel::DEBUG, info, "Already in close list, ignoring duplicate.");
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
            logger_->log(LogLevel::ERROR, "Failed to accept client connection.");
            throw std::runtime_error("Failed to accept client connection.");
        }

        // 设置客户端 socket 为非阻塞
        setNonBlocking(client_fd);

        if (linger_) {
            linger so_linger{};
            so_linger.l_onoff = 1;
            so_linger.l_linger = 1;
            setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
        }

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

        logger_->log(LogLevel::INFO, info, "New client connected.");
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
            logger_->log(LogLevel::WARNING, info, "Read on invalid fd: already closed elsewhere.");
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            logger_->log(LogLevel::ERROR, info, std::format("Read error: {}", strerror(errno)));
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

    std::string response;

    // 根据方法和路径进行不同的处理
    if (method == "GET") {
        logger_->log(LogLevel::DEBUG, info, std::format("Handling GET for path: {}", path));
        response = static_file_.serve(path, info);
    } else if (method == "POST") {
        logger_->log(LogLevel::DEBUG, info, std::format("Handling POST for path: {}", path));
        response = handlePOST(request);
    } else {
        logger_->log(LogLevel::WARNING, info, std::format("Unsupported method: {} on path: {}", method, path));
        constexpr int error_code = 405;
        response = HttpResponse::buildErrorResponse(error_code);
    }

    write(client_fd, response.c_str(), response.size());
    requestCloseClient(client_fd);
}

std::string Server::handlePOST(const std::string& request) const {
    const std::string delimiter = "\r\n\r\n";
    const size_t body_pos = request.find(delimiter);
    if (body_pos == std::string::npos) {
        constexpr int error_code = 400;
        return HttpResponse::buildErrorResponse(error_code);
    }

    std::string body = request.substr(body_pos + delimiter.size());

    logger_->log(LogLevel::DEBUG, std::format("POST body: {}", body));

    auto form_data = FormPasser::parse(body);
    if (form_data.empty()) {
        return "No form data received.";
    }

    std::string result = "Received POST data:\n";
    for (const auto& [key, value] : form_data) {
        result += std::format("    {} = {}\n", key, value);
    }

    return HttpResponse{}.setStatus("200 OK").setContentType("text/plain; charset=UTF-8").setBody(result).build();
}

void Server::dispatchClient(const int client_fd) {
    try {
        thread_pool_.enqueue([this, client_fd] { handleClientData(client_fd); });
    } catch (const std::exception& e) {
        logger_->log(LogLevel::ERROR, getClientInfo(client_fd), std::format("Failed to enqueue task: {}", e.what()));
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
