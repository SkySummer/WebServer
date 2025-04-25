#include "core/server.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/connection.h"
#include "utils/logger.h"

constexpr int MAX_EVENTS = 1024;  // epoll 支持的最大事件数

inline sockaddr* toSockaddr(sockaddr_in* addr) {
    return reinterpret_cast<sockaddr*>(addr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

Server::Server(const uint16_t port, const bool linger, Logger* logger, const size_t thread_count)
    : port_(port), linger_(linger), logger_(logger), thread_pool_(thread_count, logger) {
    setupSocket();
    setupEpoll();
}

Server::~Server() {
    close(listen_fd_);
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

void Server::setupEpoll() const {
    try {
        epoll_manager_.addFd(listen_fd_, EPOLLIN | EPOLLET);
        logger_->log(LogLevel::INFO, "Epoll instance created and listening socket added.");
    } catch (const std::exception& e) {
        logger_->log(LogLevel::ERROR, std::format("Epoll setup failed: {}", e.what()));
        throw;
    }
}

void Server::run() {
    logger_->logDivider("Server start");

    std::array<epoll_event, MAX_EVENTS> events{};
    while (true) {
        const int event_count = epoll_manager_.wait(events, -1);
        for (int i = 0; i < event_count; ++i) {
            if (const int client_fd = events.at(i).data.fd; client_fd == listen_fd_) {
                handleNewConnection();
            } else {
                dispatchClient(client_fd);
            }
        }
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

        const auto conn =
            std::make_shared<Connection>(client_fd, client_addr, &epoll_manager_, logger_, &static_file_, linger_);

        if (!conn) {
            logger_->log(LogLevel::ERROR, "Failed to create connection object.");
            close(client_fd);
            continue;
        }

        conn->setCloseRequestCallback([this](const int close_fd) {
            std::lock_guard lock(connections_mutex_);
            connections_.erase(close_fd);
        });

        std::lock_guard lock(connections_mutex_);
        connections_[client_fd] = conn;
    }
}

void Server::dispatchClient(const int client_fd) {
    std::shared_ptr<Connection> conn;
    {
        std::lock_guard lock(connections_mutex_);
        const auto iter = connections_.find(client_fd);
        if (iter == connections_.end()) {
            return;
        }
        conn = iter->second;
    }

    if (!conn) {
        logger_->log(LogLevel::WARNING, "Connection is null, cannot dispatch.");
        return;
    }

    try {
        thread_pool_.enqueue([conn] { conn->handle(); });
    } catch (const std::exception& e) {
        logger_->log(LogLevel::ERROR, conn->info(), std::format("Failed to enqueue task: {}", e.what()));
    }
}

int Server::setNonBlocking(const int socket_fd) {
    const int old_flags = fcntl(socket_fd, F_GETFL);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    return fcntl(socket_fd, F_SETFL, old_flags | O_NONBLOCK);  // NOLINT(cppcoreguidelines-pro-type-vararg)
}
