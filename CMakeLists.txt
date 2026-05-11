cmake_minimum_required(VERSION 3.16)
project(TcpServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Derleyici uyarıları
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(tcp_server
    main.cpp
    Server.cpp
    ClientHandler.cpp
)

# POSIX thread desteği
find_package(Threads REQUIRED)
target_link_libraries(tcp_server PRIVATE Threads::Threads)
