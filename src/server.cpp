#include "server.h"

#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

constexpr int MAX_EVENTS = 1024; // epoll 支持的最大事件数

// 设置文件描述符为非阻塞模式
static int setNonBlocking(const int fd) {
    const int old = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, old | O_NONBLOCK);
}

// 构造函数：初始化服务器（创建 socket + epoll）
Server::Server(const int port) : port_(port) {
    setupSocket();
    setupEpoll();
}

// 析构函数：关闭监听 socket 和 epoll 文件描述符
Server::~Server() {
    close(listen_fd_);
    close(epoll_fd_);
}

// 创建 socket，绑定地址和端口，设置监听
void Server::setupSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        perror("socket");
        exit(1);
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
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // 开始监听连接请求
    if (listen(listen_fd_, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }

    // 设置监听 socket 为非阻塞
    setNonBlocking(listen_fd_);

    std::cout << "Server listening on port " << port_ << std::endl;
}

// 创建 epoll 实例并添加监听 socket 到 epoll 中
void Server::setupEpoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        perror("epoll_create1");
        exit(1);
    }

    // 添加监听 socket 到 epoll 监听列表
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
}

// 主事件循环：等待事件并分发处理
void Server::run() const {
    while (true) {
        epoll_event events[MAX_EVENTS];
        const int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (const int fd = events[i].data.fd; fd == listen_fd_) {
                handleNewConnection();
            } else {
                handleClientData(fd);
            }
        }
    }
}

// 处理新客户端连接
void Server::handleNewConnection() const {
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    const int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
    if (client_fd == -1) {
        perror("accept");
        exit(1);
    }

    // 设置客户端 socket 为非阻塞
    setNonBlocking(client_fd);

    // 添加客户端 socket 到 epoll 中，监听读写事件
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    std::cout << "Client connected, fd = " << client_fd << std::endl;
}

// 处理客户端发送的数据（简单解析请求路径，根据路径返回不同内容）
void Server::handleClientData(const int client_fd) {
    char buf[4096];
    if (const ssize_t n = read(client_fd, buf, sizeof(buf)); n == 0) {
        // 如果读到 0 字节，说明客户端关闭连接
        close(client_fd);
        std::cout << "Client disconnected, fd = " << client_fd << std::endl;
        return;
    } else if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            close(client_fd);
        }
        return;
    }

    // 将读取的内容转换为 std::string 以便处理
    std::string request(buf);
    std::string path = "/";

    // 解析 HTTP 请求行中的路径部分（格式如：GET /path HTTP/1.1）
    if (const size_t method_end = request.find(' '); method_end != std::string::npos) {
        if (const size_t path_end = request.find(' ', method_end + 1); path_end != std::string::npos) {
            path = request.substr(method_end + 1, path_end - method_end - 1);
        }
    }

    // 根据路径构造不同的响应体内容
    std::string body;
    if (path == "/") {
        body = "Welcome to the C++ WebServer!";
    } else if (path == "/hello") {
        body = "Hello, world!";
    } else {
        body = "404 Not Found";
    }

    // 发送固定的 HTTP 响应（状态行 + 头部 + 空行 + 内容）
    const auto response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    write(client_fd, response.c_str(), response.size());
}
