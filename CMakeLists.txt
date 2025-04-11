cmake_minimum_required(VERSION 3.13)
project(WebServer LANGUAGES CXX)

# 设置全局 C++ 标准为 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 定义源代码和头文件目录
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

# 递归收集所有源文件
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${SRC_DIR}/*.cpp"
)

# 定义可执行文件目标
add_executable(WebServer
        main.cpp
        ${SRC_FILES}
)

# 设置头文件包含目录
target_include_directories(WebServer
        PRIVATE
        ${INCLUDE_DIR}
)

# 启用常见警告、额外警告和标准严格检查
target_compile_options(WebServer PRIVATE -Wall -Wextra -Wpedantic)
