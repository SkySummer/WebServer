#include "core/connection.h"

#include <cstring>
#include <format>
#include <string>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

#include "core/epoll_manager.h"
#include "core/static_file.h"
#include "utils/form_parser.h"
#include "utils/logger.h"

Connection::Connection(const int client_fd, const sockaddr_in& addr, EpollManager* epoll, Logger* logger,
                       StaticFile* static_file, const bool linger)
    : client_fd_(client_fd), info_(addr, client_fd), epoll_manager_(epoll), logger_(logger), static_file_(static_file) {
    // 设置 linger 选项
    applyLinger(linger);

    // 将客户端 socket 添加到 epoll 中，监听读写事件
    epoll_manager_->addFd(client_fd_, EPOLLIN);

    logger_->log(LogLevel::INFO, info_, "New client connected.");
}

Connection::~Connection() {
    closeConnection();
}

int Connection::fd() const {
    return client_fd_;
}

const Address& Connection::info() const {
    return info_;
}

void Connection::handle() {
    readAndHandleRequest();
}

void Connection::readAndHandleRequest() {
    if (closed_) {
        logger_->log(LogLevel::WARNING, info_, "Connection already closed.");
        return;
    }

    constexpr std::size_t buffer_size = 4096;
    std::array<char, buffer_size> buffer{};  // 用于存储从客户端接收到的数据
    const ssize_t bytes_read = read(client_fd_, buffer.data(), buffer.size());

    if (bytes_read == 0) {
        // 如果读到 0 字节，说明客户端关闭连接
        if (callback_) {
            callback_(client_fd_);
        }
        return;
    }

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;  // 没有更多数据可读
        }
        if (errno == ECONNRESET) {
            logger_->log(LogLevel::INFO, info_, "Connection reset by peer.");
        } else {
            logger_->log(LogLevel::ERROR, info_, std::format("Failed to read from client: {}", strerror(errno)));
        }

        if (callback_) {
            callback_(client_fd_);
        }
        return;
    }

    // 将读取的内容转换为 std::string 以便处理
    const std::string request(buffer.data(), bytes_read);
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
        logger_->log(LogLevel::DEBUG, info_, std::format("Handling GET for path: {}", path));
        response = handleGetRequest(path);
    } else if (method == "POST") {
        logger_->log(LogLevel::DEBUG, info_, std::format("Handling POST for path: {}", path));

        static constexpr std::string_view delimiter = "\r\n\r\n";
        const size_t body_pos = request.find(delimiter);
        if (body_pos == std::string::npos) {
            constexpr int error_code = 400;
            response = HttpResponse::buildErrorResponse(error_code);
        } else {
            std::string body = request.substr(body_pos + delimiter.size());
            response = handlePostRequest(path, body);
        }
    } else {
        logger_->log(LogLevel::DEBUG, info_, std::format("Unsupported method: {} on path: {}", method, path));
        constexpr int error_code = 405;
        response = HttpResponse::buildErrorResponse(error_code);
    }

    write(client_fd_, response.c_str(), response.size());

    if (callback_) {
        callback_(client_fd_);
    }
}

std::string Connection::handleGetRequest(const std::string& path) const {
    return static_file_->serve(path, info_);
}

std::string Connection::handlePostRequest(const std::string& path, const std::string& body) {
    auto form_data = FormPasser::parse(body);
    if (form_data.empty()) {
        constexpr int error_code = 400;
        return HttpResponse::buildErrorResponse(error_code, "No form data received.");
    }

    std::string result = std::format("Received POST data from {}:\n", path);
    for (const auto& [key, value] : form_data) {
        result += std::format("    {} = {}\n", key, value);
    }

    return HttpResponse{}.setStatus("200 OK").setContentType("text/plain; charset=UTF-8").setBody(result).build();
}

void Connection::closeConnection() {
    if (closed_.exchange(true)) {
        return;
    }

    // 从 epoll 中删除客户端 socket
    epoll_manager_->delFd(client_fd_);
    close(client_fd_);

    logger_->log(LogLevel::INFO, info_, "Client disconnected.");
}

void Connection::applyLinger(const bool flag) const {
    if (flag) {
        linger so_linger{};
        so_linger.l_onoff = 1;
        so_linger.l_linger = 1;
        setsockopt(client_fd_, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    }
}

void Connection::setCloseRequestCallback(std::function<void(int)> callback) {
    callback_ = std::move(callback);
}
