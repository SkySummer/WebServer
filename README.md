# WebServer

本项目是一个使用现代 C++20 编写的高性能轻量级服务器。

## 当前功能

- 支持 `GET` 和 `POST` 请求方法。
- 支持处理多个 HTTP 状态码，例如 `200 OK`、`404 Not Found`、`405 Method Not Allowed`。
- 支持简单的 URL 路由，根据不同路径返回不同的内容。
- 采用 **多线程** 模型处理客户端请求，提高并发性能。
- 实现 **非阻塞 I/O** 和 **epoll** 事件驱动模型，提升性能和响应速度。

## 项目结构

```
WebServer/
├── include/
│   ├── server.h
│   └── threadpool.h
├── src/
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

## 许可证

本项目基于 MIT License 开源，详见 [LICENSE](./LICENSE)。
