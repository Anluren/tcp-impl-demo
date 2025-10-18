#include "tcp_socket.h"
#include <iostream>
#include <string>
#include <cstring>

using namespace tcp_stack;

int main() {
    std::cout << "TCP Server Starting..." << std::endl;
    
    try {
        TCPSocket server_socket;
        
        // Bind to localhost on port 8080
        if (!server_socket.bind("127.0.0.1", 8080)) {
            std::cerr << "Failed to bind socket" << std::endl;
            return 1;
        }
        
        // Start listening for connections
        if (!server_socket.listen(5)) {
            std::cerr << "Failed to listen on socket" << std::endl;
            return 1;
        }
        
        std::cout << "Server listening on 127.0.0.1:8080" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        // Accept and handle connections
        while (true) {
            auto client_socket = server_socket.accept();
            if (client_socket) {
                std::cout << "Client connected from " 
                         << client_socket->get_remote_address() 
                         << ":" << client_socket->get_remote_port() << std::endl;
                
                // Simple echo server
                char buffer[1024];
                while (client_socket->is_connected()) {
                    ssize_t bytes_received = client_socket->recv(buffer, sizeof(buffer) - 1);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        std::cout << "Received: " << buffer << std::endl;
                        
                        // Echo back to client
                        std::string response = "Echo: " + std::string(buffer);
                        client_socket->send(response.c_str(), response.length());
                    } else if (bytes_received == 0) {
                        std::cout << "Client disconnected" << std::endl;
                        break;
                    }
                }
                
                client_socket->close();
            }
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}