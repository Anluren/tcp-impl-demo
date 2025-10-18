# Unit tests
find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(tcp_tests test_tcp.cpp)
    target_link_libraries(tcp_tests tcp_stack gtest gtest_main)
    add_test(NAME tcp_tests COMMAND tcp_tests)
    
    add_executable(local_socket_tests test_local_sockets.cpp)
    target_link_libraries(local_socket_tests tcp_stack gtest gtest_main)
    add_test(NAME local_socket_tests COMMAND local_socket_tests)
else()
    # Simple test executables without Google Test
    add_executable(tcp_tests test_tcp.cpp)
    target_link_libraries(tcp_tests tcp_stack)
    add_test(NAME tcp_tests COMMAND tcp_tests)
    
    add_executable(local_socket_tests test_local_sockets.cpp)
    target_link_libraries(local_socket_tests tcp_stack)
    add_test(NAME local_socket_tests COMMAND local_socket_tests)
    
    message(STATUS "Google Test not found, building simple test executables")
endif()