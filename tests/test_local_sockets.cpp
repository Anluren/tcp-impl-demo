#include "local_tcp_socket.h"
#include "tcp_state_machine.h"
#include "network_utils.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace tcp_stack;

void test_local_socket_basic() {
    std::cout << "=== Testing Local Socket Basic Operations ===" << std::endl;
    
    LocalTCPSocket socket;
    assert(!socket.is_connected());
    
    // Test binding
    bool bind_result = socket.bind("127.0.0.1", 0); // Use port 0 for any available port
    std::cout << "Bind result: " << (bind_result ? "SUCCESS" : "FAILED") << std::endl;
    assert(bind_result);
    
    std::cout << "Local socket basic operations: PASSED" << std::endl;
}

void test_local_socket_echo() {
    std::cout << "\n=== Testing Local Socket Echo Client/Server ===" << std::endl;
    
    const uint16_t test_port = 9999;
    bool server_ready = false;
    bool test_completed = false;
    std::string server_error, client_error;
    
    // Server thread
    std::thread server_thread([&]() {
        try {
            LocalTCPSocket server;
            if (!server.bind("127.0.0.1", test_port)) {
                server_error = "Server bind failed";
                return;
            }
            
            if (!server.listen(1)) {
                server_error = "Server listen failed";
                return;
            }
            
            server_ready = true;
            std::cout << "Test server ready on port " << test_port << std::endl;
            
            // Accept one connection
            auto client = server.accept();
            if (!client) {
                server_error = "Server accept failed";
                return;
            }
            
            std::cout << "Test server accepted connection" << std::endl;
            
            // Echo one message
            char buffer[1024];
            ssize_t bytes = client->recv(buffer, sizeof(buffer));
            if (bytes > 0) {
                std::string message(buffer, bytes);
                std::cout << "Test server received: " << message << std::endl;
                
                std::string response = "Echo: " + message;
                client->send(response.c_str(), response.length());
                std::cout << "Test server sent echo response" << std::endl;
            }
            
            client->close();
            
        } catch (const std::exception& e) {
            server_error = std::string("Server exception: ") + e.what();
        }
    });
    
    // Client thread
    std::thread client_thread([&]() {
        try {
            // Wait for server to be ready
            while (!server_ready && server_error.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            if (!server_error.empty()) {
                client_error = "Server failed to start";
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give server time to accept
            
            LocalTCPSocket client;
            if (!client.connect("127.0.0.1", test_port)) {
                client_error = "Client connect failed";
                return;
            }
            
            std::cout << "Test client connected" << std::endl;
            
            // Send test message
            std::string test_message = "Hello, Test!";
            ssize_t sent = client.send(test_message.c_str(), test_message.length());
            if (sent <= 0) {
                client_error = "Client send failed";
                return;
            }
            
            std::cout << "Test client sent: " << test_message << std::endl;
            
            // Receive echo
            char response[1024];
            ssize_t received = client.recv(response, sizeof(response));
            if (received > 0) {
                std::string echo_response(response, received);
                std::cout << "Test client received: " << echo_response << std::endl;
                
                if (echo_response == "Echo: " + test_message) {
                    test_completed = true;
                    std::cout << "Echo test: SUCCESS" << std::endl;
                } else {
                    client_error = "Echo response mismatch";
                }
            } else {
                client_error = "Client receive failed";
            }
            
            client.close();
            
        } catch (const std::exception& e) {
            client_error = std::string("Client exception: ") + e.what();
        }
    });
    
    // Wait for threads to complete
    server_thread.join();
    client_thread.join();
    
    if (!server_error.empty()) {
        std::cout << "Server error: " << server_error << std::endl;
    }
    if (!client_error.empty()) {
        std::cout << "Client error: " << client_error << std::endl;
    }
    
    assert(test_completed && server_error.empty() && client_error.empty());
    std::cout << "Local socket echo test: PASSED" << std::endl;
}

void test_tcp_stack_components() {
    std::cout << "\n=== Testing TCP Stack Components ===" << std::endl;
    
    // Test state machine (already tested in main test)
    TCPStateMachine sm;
    assert(sm.get_state() == TCPState::CLOSED);
    sm.process_event(TCPEvent::PASSIVE_OPEN);
    assert(sm.get_state() == TCPState::LISTEN);
    std::cout << "TCP State Machine: PASSED" << std::endl;
    
    // Test network utilities
    uint32_t ip = NetworkUtils::ip_string_to_network("192.168.1.1");
    std::string ip_str = NetworkUtils::ip_network_to_string(ip);
    assert(ip_str == "192.168.1.1");
    std::cout << "Network Utils: PASSED" << std::endl;
    
    std::cout << "TCP stack components: PASSED" << std::endl;
}

void test_performance_comparison() {
    std::cout << "\n=== Performance Comparison Info ===" << std::endl;
    std::cout << "Local sockets use kernel TCP stack (optimized)" << std::endl;
    std::cout << "Our TCP stack uses raw sockets (educational)" << std::endl;
    std::cout << "Local sockets: Higher performance, system integration" << std::endl;
    std::cout << "Our TCP stack: Learning, customization, protocol understanding" << std::endl;
}

int main() {
    std::cout << "Running TCP Stack Local Socket Tests..." << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        test_local_socket_basic();
        test_local_socket_echo();
        test_tcp_stack_components();
        test_performance_comparison();
        
        std::cout << "===========================================" << std::endl;
        std::cout << "All local socket tests completed successfully!" << std::endl;
        
        std::cout << "\nTo test interactively:" << std::endl;
        std::cout << "1. Run: ./examples/local_server" << std::endl;
        std::cout << "2. In another terminal: ./examples/local_client" << std::endl;
        std::cout << "3. Type messages and see them echoed back!" << std::endl;
        
        std::cout << "\nLocal sockets vs Our TCP Stack:" << std::endl;
        std::cout << "- Local sockets: Use system TCP (no root required)" << std::endl;
        std::cout << "- Our TCP stack: Educational raw socket implementation" << std::endl;
        std::cout << "- Both demonstrate TCP protocol concepts!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}