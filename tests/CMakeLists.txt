# Unit tests
find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(tcp_tests test_tcp.cpp)
    target_link_libraries(tcp_tests tcp_stack gtest gtest_main)
    add_test(NAME tcp_tests COMMAND tcp_tests)
else()
    # Simple test executable without Google Test
    add_executable(tcp_tests test_tcp.cpp)
    target_link_libraries(tcp_tests tcp_stack)
    add_test(NAME tcp_tests COMMAND tcp_tests)
    message(STATUS "Google Test not found, building simple test executable")
endif()