#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <threadpool.h>

// Server 类用于创建一个基于 epoll 的多路复用 Web 服务器
class Server {
public:
    // 构造函数：初始化服务器并指定监听端口
    explicit Server(int port, int thread_count);

    // 析构函数：关闭 socket 与 epoll 相关资源
    ~Server();

    // 启动服务器主循环，开始处理客户端请求
    void run();

private:
    int port_; // 服务器监听端口
    int listen_fd_{}; // 监听 socket 文件描述符
    int epoll_fd_{}; // epoll 实例的文件描述符

    ThreadPool thread_pool_; // 线程池

    // 创建并配置 socket，绑定端口并监听连接
    void setupSocket();

    // 创建 epoll 实例并添加监听 socket
    void setupEpoll();

    // 处理新客户端连接
    void handleNewConnection() const;

    // 处理客户端发送的数据
    static void handleClientData(int client_fd);

    // 处理 GET 请求，根据路径返回不同的响应内容
    static std::string handleGET(const std::string& path);

    // 处理 POST 请求，读取请求体并返回内容
    static std::string handlePOST(const std::string& path, const std::string& request);

    // 分发任务
    void dispatchClient(int client_fd);
};

#endif //SERVER_H
