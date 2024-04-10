cmake_minimum_required(VERSION 3.27)
project(WebServer)

set(CMAKE_CXX_STANDARD 14)

LINK_DIRECTORIES(/usr/local/lib)

add_executable(WebServer
        main.cpp
        ThreadPool.cpp
        ThreadPool.h
        libeventHttp.cpp
        libeventHttp.h
        http.cpp
        http.h
        headfile.h
)
TARGET_LINK_LIBRARIES(WebServer event)
