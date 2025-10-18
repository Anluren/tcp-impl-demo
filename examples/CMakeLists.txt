# TCP stack examples (require root privileges)
add_executable(tcp_server server.cpp)
target_link_libraries(tcp_server tcp_stack)

add_executable(tcp_client client.cpp)
target_link_libraries(tcp_client tcp_stack)

# Local socket examples (no root required)
add_executable(local_server local_server.cpp)
target_link_libraries(local_server tcp_stack)

add_executable(local_client local_client.cpp)
target_link_libraries(local_client tcp_stack)