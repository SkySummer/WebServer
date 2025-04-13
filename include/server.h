#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <threadpool.h>
#include <unordered_map>
#include <unordered_set>

#include "address.h"
#include "logger.h"

class Server {
public:
    // 构造函数：初始化服务器并指定监听端口
    explicit Server(int port, int thread_count, Logger& logger);

    // 析构函数：关闭 socket 与 epoll 相关资源
    ~Server();

    // 启动服务器主循环，开始处理客户端请求
    void run();

private:
    int port_; // 服务器监听端口
    int listen_fd_{}; // 监听 socket 文件描述符
    int epoll_fd_{}; // epoll 实例的文件描述符
    ThreadPool thread_pool_; // 线程池
    Logger& logger_; // 日志

    std::unordered_map<int, Address> clients_; // 缓存客户端地址
    std::mutex clients_mutex_;

    std::unordered_set<int> close_list_; // 客户端关闭列表
    std::mutex close_mutex_;

    // 创建并配置 socket，绑定端口并监听连接
    void setupSocket();

    // 创建 epoll 实例并添加监听 socket
    void setupEpoll();

    // 处理新客户端连接
    void handleNewConnection();

    // 请求客户端断开
    void requestCloseClient(int client_fd);

    // 处理关闭列表
    void processCloseList();

    // 处理客户端断开
    void disconnectClient(int client_fd);

    // 处理客户端发送的数据
    void handleClientData(int client_fd);

    // 处理 GET 请求，根据路径返回不同的响应内容
    [[nodiscard]] static std::string handleGET(const std::string& path);

    // 处理 POST 请求，读取请求体并返回内容
    [[nodiscard]] std::string handlePOST(const std::string& path, const std::string& request) const;

    // 分发任务
    void dispatchClient(int client_fd);

    // 获取客户端信息
    [[nodiscard]] std::string getClientInfo(int client_fd);

    // 设置为非阻塞模式
    static int setNonBlocking(int fd);
};

#endif //SERVER_H
