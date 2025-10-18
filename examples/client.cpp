#include "tcp_socket.h"
#include <iostream>
#include <string>

using namespace tcp_stack;

int main() {
    std::cout << "TCP Client Starting..." << std::endl;
    
    try {
        TCPSocket client_socket;
        
        // Connect to server
        if (!client_socket.connect("127.0.0.1", 8080)) {
            std::cerr << "Failed to connect to server" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to server at 127.0.0.1:8080" << std::endl;
        std::cout << "Type messages to send (type 'quit' to exit):" << std::endl;
        
        // Interactive client
        std::string input;
        char response_buffer[1024];
        
        while (client_socket.is_connected()) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input == "quit") {
                break;
            }
            
            if (!input.empty()) {
                // Send message to server
                ssize_t bytes_sent = client_socket.send(input.c_str(), input.length());
                if (bytes_sent > 0) {
                    std::cout << "Sent " << bytes_sent << " bytes" << std::endl;
                    
                    // Receive response
                    ssize_t bytes_received = client_socket.recv(response_buffer, sizeof(response_buffer) - 1);
                    if (bytes_received > 0) {
                        response_buffer[bytes_received] = '\0';
                        std::cout << "Server response: " << response_buffer << std::endl;
                    }
                } else {
                    std::cerr << "Failed to send message" << std::endl;
                    break;
                }
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