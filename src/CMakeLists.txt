project(Sources)

add_executable(ssloff
    server.cpp
)

target_link_libraries(ssloff
    asio::asio
    openssl::openssl
)
