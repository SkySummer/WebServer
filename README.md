# WebServer

本项目是一个使用现代 C++20 编写的高性能轻量级服务器。

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
