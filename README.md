# WebServer

本项目是一个使用现代 C++20 编写的高性能轻量级服务器。

## 当前功能

- 支持处理 `GET` 和 `POST` 请求
- 根据不同路径返回不同的响应内容
  - `/` : 欢迎页面
  - `/hello` : 返回 `Hello, world!`
  - 其他路径 : 返回 `404 Not Found`
- 简单支持读取 `POST` 请求体
- 处理 HTTP 状态码：
  - 200 OK
  - 405 Method Not Allowed（针对非 GET 请求）
  - 404 Not Found（未匹配到路径）
- 使用 `epoll` 实现高并发的网络连接处理

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
