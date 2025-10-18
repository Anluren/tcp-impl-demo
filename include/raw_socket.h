#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace tcp_stack {

class RawSocket {
public:
    RawSocket();
    ~RawSocket();
    
    // Non-copyable but movable
    RawSocket(const RawSocket&) = delete;
    RawSocket& operator=(const RawSocket&) = delete;
    RawSocket(RawSocket&&) noexcept;
    RawSocket& operator=(RawSocket&&) noexcept;
    
    // Initialize the raw socket
    bool initialize();
    
    // Close the socket
    void close();
    
    // Send raw IP packet
    bool send_packet(const std::vector<uint8_t>& packet, uint32_t dst_ip);
    
    // Receive raw IP packet
    bool receive_packet(std::vector<uint8_t>& packet, uint32_t& src_ip);
    
    // Set socket to non-blocking mode
    bool set_non_blocking(bool non_blocking = true);
    
    // Check if socket is valid
    bool is_valid() const { return socket_fd_ != -1; }
    
    // Get socket file descriptor
    int get_fd() const { return socket_fd_; }
    
private:
    int socket_fd_;
    bool initialized_;
    
    // Platform-specific initialization
    bool create_raw_socket();
    bool configure_socket();
};

} // namespace tcp_stack