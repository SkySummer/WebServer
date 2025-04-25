#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "core/epoll_manager.h"
#include "core/static_file.h"
#include "core/threadpool.h"

// 前向声明
class Connection;
class Logger;

class Server {
public:
    // 构造函数：初始化服务器并指定监听端口
    explicit Server(uint16_t port, bool linger, Logger* logger, size_t thread_count);

    // 析构函数：关闭 socket 与 epoll 相关资源
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    // 启动服务器主循环，开始处理客户端请求
    void run();

private:
    const uint16_t port_;  // 服务器监听端口
    int listen_fd_{};      // 监听 socket 文件描述符
    const bool linger_;    // 是否启用 linger 模式

    std::unordered_map<int, std::shared_ptr<Connection>> connections_;  // 客户端连接列表
    std::mutex connections_mutex_;

    Logger* logger_;                               // 日志
    EpollManager epoll_manager_;                   // epoll 管理器
    ThreadPool thread_pool_;                       // 线程池
    StaticFile static_file_{logger_, "./static"};  // 静态文件目录

    // 创建并配置 socket，绑定端口并监听连接
    void setupSocket();

    // 创建 epoll 实例并添加监听 socket
    void setupEpoll() const;

    // 处理新客户端连接
    void handleNewConnection();

    // 分发任务
    void dispatchClient(int client_fd);

    // 设置为非阻塞模式
    static int setNonBlocking(int socket_fd);
};

#endif  // CORE_SERVER_H
