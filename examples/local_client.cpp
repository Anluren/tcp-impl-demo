#include "local_tcp_socket.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace tcp_stack;

int main() {
    std::cout << "Local TCP Client Test Starting..." << std::endl;
    
    try {
        LocalTCPSocket client_socket;
        
        // Connect to server
        std::cout << "Connecting to server at 127.0.0.1:9090..." << std::endl;
        if (!client_socket.connect("127.0.0.1", 9090)) {
            std::cerr << "Failed to connect to server" << std::endl;
            std::cerr << "Make sure the server is running first!" << std::endl;
            return 1;
        }
        
        std::cout << "Connected successfully!" << std::endl;
        std::cout << "Local address: " << client_socket.get_local_address() 
                  << ":" << client_socket.get_local_port() << std::endl;
        std::cout << "Remote address: " << client_socket.get_remote_address()
                  << ":" << client_socket.get_remote_port() << std::endl;
        
        std::cout << "\nType messages to send (type 'quit' to exit):" << std::endl;
        
        // Interactive client
        std::string input;
        char response_buffer[1024];
        
        while (client_socket.is_connected()) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input.empty()) {
                continue;
            }
            
            if (input == "quit") {
                // Send quit command to server
                ssize_t bytes_sent = client_socket.send(input.c_str(), input.length());
                std::cout << "Sent quit command (" << bytes_sent << " bytes)" << std::endl;
                
                // Wait for response
                ssize_t bytes_received = client_socket.recv(response_buffer, sizeof(response_buffer) - 1);
                if (bytes_received > 0) {
                    response_buffer[bytes_received] = '\0';
                    std::cout << "Server response: " << response_buffer << std::endl;
                }
                break;
            }
            
            // Send message to server
            ssize_t bytes_sent = client_socket.send(input.c_str(), input.length());
            if (bytes_sent > 0) {
                std::cout << "Sent " << bytes_sent << " bytes" << std::endl;
                
                // Receive response
                ssize_t bytes_received = client_socket.recv(response_buffer, sizeof(response_buffer) - 1);
                if (bytes_received > 0) {
                    response_buffer[bytes_received] = '\0';
                    std::cout << "Server response (" << bytes_received << " bytes): " 
                              << response_buffer << std::endl;
                } else if (bytes_received == 0) {
                    std::cout << "Server closed connection" << std::endl;
                    break;
                } else {
                    std::cout << "Failed to receive response" << std::endl;
                }
            } else {
                std::cerr << "Failed to send message" << std::endl;
                break;
            }
        }
        
        client_socket.close();
        std::cout << "Connection closed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}