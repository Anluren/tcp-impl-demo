#pragma once

#include "tcp_socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>

namespace tcp_stack {

// Wrapper for standard TCP sockets for testing purposes
class LocalTCPSocket {
public:
    LocalTCPSocket();
    ~LocalTCPSocket();
    
    // Non-copyable but movable
    LocalTCPSocket(const LocalTCPSocket&) = delete;
    LocalTCPSocket& operator=(const LocalTCPSocket&) = delete;
    LocalTCPSocket(LocalTCPSocket&&) noexcept;
    LocalTCPSocket& operator=(LocalTCPSocket&&) noexcept;
    
    // Socket operations (similar to our TCP stack API)
    bool bind(const std::string& ip_address, uint16_t port);
    bool listen(int backlog = 5);
    std::unique_ptr<LocalTCPSocket> accept();
    bool connect(const std::string& ip_address, uint16_t port);
    
    // Data transfer
    ssize_t send(const void* data, size_t length);
    ssize_t recv(void* buffer, size_t length);
    
    // Socket management
    bool close();
    bool is_connected() const;
    
    // Socket options
    bool set_blocking(bool blocking);
    bool set_receive_timeout(std::chrono::milliseconds timeout);
    
    // Get socket information
    std::string get_local_address() const;
    uint16_t get_local_port() const;
    std::string get_remote_address() const;
    uint16_t get_remote_port() const;
    
private:
    // Internal constructor for accepted connections
    LocalTCPSocket(int accepted_fd, const sockaddr_in& client_addr);
    
    int socket_fd_;
    bool is_listening_;
    bool is_connected_;
    
    sockaddr_in local_addr_;
    sockaddr_in remote_addr_;
    
    // Helper methods
    bool create_socket();
    std::string sockaddr_to_string(const sockaddr_in& addr) const;
    uint16_t get_port_from_sockaddr(const sockaddr_in& addr) const;
};

} // namespace tcp_stack