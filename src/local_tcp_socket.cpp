#include "local_tcp_socket.h"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

namespace tcp_stack {

LocalTCPSocket::LocalTCPSocket() 
    : socket_fd_(-1), is_listening_(false), is_connected_(false) {
    std::memset(&local_addr_, 0, sizeof(local_addr_));
    std::memset(&remote_addr_, 0, sizeof(remote_addr_));
}

LocalTCPSocket::~LocalTCPSocket() {
    close();
}

LocalTCPSocket::LocalTCPSocket(LocalTCPSocket&& other) noexcept
    : socket_fd_(other.socket_fd_), is_listening_(other.is_listening_),
      is_connected_(other.is_connected_), local_addr_(other.local_addr_),
      remote_addr_(other.remote_addr_) {
    other.socket_fd_ = -1;
    other.is_listening_ = false;
    other.is_connected_ = false;
}

LocalTCPSocket& LocalTCPSocket::operator=(LocalTCPSocket&& other) noexcept {
    if (this != &other) {
        close();
        socket_fd_ = other.socket_fd_;
        is_listening_ = other.is_listening_;
        is_connected_ = other.is_connected_;
        local_addr_ = other.local_addr_;
        remote_addr_ = other.remote_addr_;
        
        other.socket_fd_ = -1;
        other.is_listening_ = false;
        other.is_connected_ = false;
    }
    return *this;
}

LocalTCPSocket::LocalTCPSocket(int accepted_fd, const sockaddr_in& client_addr)
    : socket_fd_(accepted_fd), is_listening_(false), is_connected_(true),
      remote_addr_(client_addr) {
    std::memset(&local_addr_, 0, sizeof(local_addr_));
    
    // Get local address
    socklen_t addr_len = sizeof(local_addr_);
    getsockname(socket_fd_, reinterpret_cast<sockaddr*>(&local_addr_), &addr_len);
}

bool LocalTCPSocket::bind(const std::string& ip_address, uint16_t port) {
    if (!create_socket()) {
        return false;
    }
    
    local_addr_.sin_family = AF_INET;
    local_addr_.sin_port = htons(port);
    
    if (inet_aton(ip_address.c_str(), &local_addr_.sin_addr) == 0) {
        std::cerr << "Invalid IP address: " << ip_address << std::endl;
        return false;
    }
    
    // Enable address reuse
    int reuse = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
    }
    
    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&local_addr_), sizeof(local_addr_)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "LocalTCPSocket bound to " << ip_address << ":" << port << std::endl;
    return true;
}

bool LocalTCPSocket::listen(int backlog) {
    if (socket_fd_ == -1) {
        std::cerr << "Socket not created" << std::endl;
        return false;
    }
    
    if (::listen(socket_fd_, backlog) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    is_listening_ = true;
    std::cout << "LocalTCPSocket listening with backlog " << backlog << std::endl;
    return true;
}

std::unique_ptr<LocalTCPSocket> LocalTCPSocket::accept() {
    if (!is_listening_) {
        return nullptr;
    }
    
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = ::accept(socket_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        }
        return nullptr;
    }
    
    std::cout << "LocalTCPSocket accepted connection from " 
              << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;
    
    return std::unique_ptr<LocalTCPSocket>(new LocalTCPSocket(client_fd, client_addr));
}

bool LocalTCPSocket::connect(const std::string& ip_address, uint16_t port) {
    if (!create_socket()) {
        return false;
    }
    
    remote_addr_.sin_family = AF_INET;
    remote_addr_.sin_port = htons(port);
    
    if (inet_aton(ip_address.c_str(), &remote_addr_.sin_addr) == 0) {
        std::cerr << "Invalid IP address: " << ip_address << std::endl;
        return false;
    }
    
    if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&remote_addr_), sizeof(remote_addr_)) < 0) {
        std::cerr << "Connect failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    is_connected_ = true;
    
    // Get local address after connection
    socklen_t addr_len = sizeof(local_addr_);
    getsockname(socket_fd_, reinterpret_cast<sockaddr*>(&local_addr_), &addr_len);
    
    std::cout << "LocalTCPSocket connected to " << ip_address << ":" << port << std::endl;
    return true;
}

ssize_t LocalTCPSocket::send(const void* data, size_t length) {
    if (!is_connected_) {
        return -1;
    }
    
    ssize_t bytes_sent = ::send(socket_fd_, data, length, MSG_NOSIGNAL);
    if (bytes_sent < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
    }
    
    return bytes_sent;
}

ssize_t LocalTCPSocket::recv(void* buffer, size_t length) {
    if (!is_connected_) {
        return -1;
    }
    
    ssize_t bytes_received = ::recv(socket_fd_, buffer, length, 0);
    if (bytes_received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Receive failed: " << strerror(errno) << std::endl;
        }
    } else if (bytes_received == 0) {
        // Connection closed by peer
        is_connected_ = false;
    }
    
    return bytes_received;
}

bool LocalTCPSocket::close() {
    if (socket_fd_ != -1) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
    is_listening_ = false;
    is_connected_ = false;
    return true;
}

bool LocalTCPSocket::is_connected() const {
    return is_connected_;
}

bool LocalTCPSocket::set_blocking(bool blocking) {
    if (socket_fd_ == -1) {
        return false;
    }
    
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    
    return fcntl(socket_fd_, F_SETFL, flags) != -1;
}

bool LocalTCPSocket::set_receive_timeout(std::chrono::milliseconds timeout) {
    if (socket_fd_ == -1) {
        return false;
    }
    
    struct timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    
    return setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

std::string LocalTCPSocket::get_local_address() const {
    return sockaddr_to_string(local_addr_);
}

uint16_t LocalTCPSocket::get_local_port() const {
    return get_port_from_sockaddr(local_addr_);
}

std::string LocalTCPSocket::get_remote_address() const {
    return sockaddr_to_string(remote_addr_);
}

uint16_t LocalTCPSocket::get_remote_port() const {
    return get_port_from_sockaddr(remote_addr_);
}

bool LocalTCPSocket::create_socket() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

std::string LocalTCPSocket::sockaddr_to_string(const sockaddr_in& addr) const {
    return std::string(inet_ntoa(addr.sin_addr));
}

uint16_t LocalTCPSocket::get_port_from_sockaddr(const sockaddr_in& addr) const {
    return ntohs(addr.sin_port);
}

} // namespace tcp_stack