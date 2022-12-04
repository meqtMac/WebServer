cmake_minimum_required(VERSION 3.10)

project(WebServer VERSION 1.0)

add_executable(webserver main.cpp http_conn.cpp)
target_include_directories(webserver PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
