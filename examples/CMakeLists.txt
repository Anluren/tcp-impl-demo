# Simple TCP server example
add_executable(tcp_server server.cpp)
target_link_libraries(tcp_server tcp_stack)

# Simple TCP client example
add_executable(tcp_client client.cpp)
target_link_libraries(tcp_client tcp_stack)