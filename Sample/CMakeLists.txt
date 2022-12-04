cmake_minimum_required(VERSION 3.10)

project(WebServer VERSION 1.0)

add_library(httpconn src/http_conn.cpp src/http_conn.hpp src/locker.hpp src/threadpool.hpp)
target_include_directories(httpconn PUBLIC src)

add_executable(webserver 
		src/main.cpp 
		src/http_conn.hpp
		src/locker.hpp
		src/threadpool.hpp)
target_link_libraries(webserver httpconn)
target_include_directories(webserver PUBLIC src)
