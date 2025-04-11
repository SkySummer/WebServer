cmake_minimum_required(VERSION 3.30)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(WebServer main.cpp ${SRC_FILES})
