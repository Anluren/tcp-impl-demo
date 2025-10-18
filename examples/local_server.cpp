#include "local_tcp_socket.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace tcp_stack;

int main() {
    std::cout << "Local TCP Server Test Starting..." << std::endl;
    
    try {
        LocalTCPSocket server_socket;
        
        // Bind to localhost on port 9090
        if (!server_socket.bind("127.0.0.1", 9090)) {
            std::cerr << "Failed to bind socket" << std::endl;
            return 1;
        }
        
        // Start listening for connections
        if (!server_socket.listen(5)) {
            std::cerr << "Failed to listen on socket" << std::endl;
            return 1;
        }
        
        std::cout << "Local server listening on 127.0.0.1:9090" << std::endl;
        std::cout << "Waiting for connections..." << std::endl;
        
        // Accept and handle connections
        int connection_count = 0;
        while (connection_count < 3) {  // Handle up to 3 connections for testing
            auto client_socket = server_socket.accept();
            if (client_socket) {
                connection_count++;
                std::cout << "\n=== Connection #" << connection_count << " ===" << std::endl;
                std::cout << "Client connected from " 
                         << client_socket->get_remote_address() 
                         << ":" << client_socket->get_remote_port() << std::endl;
                
                // Echo server logic
                char buffer[1024];
                while (client_socket->is_connected()) {
                    ssize_t bytes_received = client_socket->recv(buffer, sizeof(buffer) - 1);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        std::cout << "Received (" << bytes_received << " bytes): " << buffer << std::endl;
                        
                        // Echo back to client with prefix
                        std::string response = "Echo: " + std::string(buffer);
                        ssize_t bytes_sent = client_socket->send(response.c_str(), response.length());
                        std::cout << "Sent (" << bytes_sent << " bytes): " << response << std::endl;
                        
                        // Check for quit command
                        if (std::string(buffer) == "quit") {
                            std::cout << "Client requested quit" << std::endl;
                            break;
                        }
                    } else if (bytes_received == 0) {
                        std::cout << "Client disconnected gracefully" << std::endl;
                        break;
                    } else {
                        // Error or would block
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
                
                client_socket->close();
                std::cout << "Connection #" << connection_count << " closed" << std::endl;
            } else {
                // No connection available, small delay
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        std::cout << "\nServer shutting down after handling " << connection_count << " connections" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}