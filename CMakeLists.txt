cmake_minimum_required(VERSION 3.23)
project(SSLOff)

find_package(GTest REQUIRED)
find_package(asio REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)

