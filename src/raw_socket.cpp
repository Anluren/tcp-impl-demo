#include "raw_socket.h"
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <iostream>

namespace tcp_stack {

RawSocket::RawSocket() : socket_fd_(-1), initialized_(false) {}

RawSocket::~RawSocket() {
    close();
}

RawSocket::RawSocket(RawSocket&& other) noexcept
    : socket_fd_(other.socket_fd_), initialized_(other.initialized_) {
    other.socket_fd_ = -1;
    other.initialized_ = false;
}

RawSocket& RawSocket::operator=(RawSocket&& other) noexcept {
    if (this != &other) {
        close();
        socket_fd_ = other.socket_fd_;
        initialized_ = other.initialized_;
        other.socket_fd_ = -1;
        other.initialized_ = false;
    }
    return *this;
}

bool RawSocket::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!create_raw_socket()) {
        std::cerr << "Failed to create raw socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (!configure_socket()) {
        std::cerr << "Failed to configure raw socket: " << strerror(errno) << std::endl;
        close();
        return false;
    }
    
    initialized_ = true;
    return true;
}

void RawSocket::close() {
    if (socket_fd_ != -1) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
    initialized_ = false;
}

bool RawSocket::send_packet(const std::vector<uint8_t>& packet, uint32_t dst_ip) {
    if (!is_valid()) {
        return false;
    }
    
    struct sockaddr_in dest_addr;
    std::memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = dst_ip;
    
    ssize_t bytes_sent = sendto(socket_fd_, packet.data(), packet.size(), 0,
                               reinterpret_cast<struct sockaddr*>(&dest_addr),
                               sizeof(dest_addr));
    
    if (bytes_sent == -1) {
        std::cerr << "Failed to send packet: " << strerror(errno) << std::endl;
        return false;
    }
    
    return bytes_sent == static_cast<ssize_t>(packet.size());
}

bool RawSocket::receive_packet(std::vector<uint8_t>& packet, uint32_t& src_ip) {
    if (!is_valid()) {
        return false;
    }
    
    packet.resize(65535); // Maximum IP packet size
    
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    
    ssize_t bytes_received = recvfrom(socket_fd_, packet.data(), packet.size(), 0,
                                     reinterpret_cast<struct sockaddr*>(&src_addr),
                                     &addr_len);
    
    if (bytes_received == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to receive packet: " << strerror(errno) << std::endl;
        }
        return false;
    }
    
    packet.resize(bytes_received);
    src_ip = src_addr.sin_addr.s_addr;
    return true;
}

bool RawSocket::set_non_blocking(bool non_blocking) {
    if (!is_valid()) {
        return false;
    }
    
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    
    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    return fcntl(socket_fd_, F_SETFL, flags) != -1;
}

bool RawSocket::create_raw_socket() {
    socket_fd_ = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    return socket_fd_ != -1;
}

bool RawSocket::configure_socket() {
    // Enable IP header inclusion (we'll construct our own IP headers)
    int one = 1;
    if (setsockopt(socket_fd_, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1) {
        return false;
    }
    
    // Set socket to non-blocking by default
    return set_non_blocking(true);
}

} // namespace tcp_stack