# WebServer

本项目是一个使用现代 C++20 编写的高性能轻量级服务器。

## 特性

- **高并发支持**：使用 epoll + 线程池的架构，能够高效处理大量并发连接。 
- **多线程处理**：通过线程池技术将客户端请求分发给工作线程，提高响应速度和吞吐量。 
- **日志系统**：内置日志模块，支持按日志等级输出日志，并支持日志文件按日期自动轮转。 
- **非阻塞 I/O**：使用非阻塞 socket 和 epoll 来进行高效的 I/O 操作，避免阻塞问题。 
- **简单的 HTTP 支持**：支持 GET 和 POST 请求，能够根据请求路径返回不同的内容。

## 目录结构

```
WebServer/
├── include/
│   ├── address.h
│   ├── logger.h
│   ├── server.h
│   └── threadpool.h
├── src/
│   ├── address.cpp
│   ├── logger.cpp
│   ├── server.cpp
│   └── threadpool.cpp
├── CMakeLists.txt
├── LICENSE
├── main.cpp
└── README.md
```

## 测试环境

- 服务器
  - Ubuntu 24.04.2 LTS
  - g++ 13.3.0
  - cmake 3.28.3

## 编译与运行方式

依赖项：`g++`, `cmake (>= 3.13)`。

```bash
mkdir build && cd build
cmake ..
make -j
./WebServer
```

## 日志

所有的日志输出会写入到 `log_YYYY-MM-DD.log` 文件中，文件按日期自动轮转。如果日志文件打开失败，程序将抛出异常并终止执行。

## 许可证

本项目基于 MIT License 开源，详见 [LICENSE](./LICENSE)。
