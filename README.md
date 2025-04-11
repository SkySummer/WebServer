# WebServer

本项目是一个使用现代 C++20 编写的高性能轻量级服务器。

## 当前功能

- 使用非阻塞 I/O 和 `epoll` 实现高效的事件驱动模型
- 支持多个客户端连接
- 解析简单的 HTTP 请求路径
- 返回固定文本的 HTTP 响应
- 支持以下路径：
  - `/`：欢迎页面
  - `/hello`：返回 Hello world
  - 其他路径返回 `"404 Not Found"`

## 项目结构

```
WebServer/
├── include/
│   └── server.h
├── src/
│   └── server.cpp
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
