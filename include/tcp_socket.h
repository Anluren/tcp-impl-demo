#pragma once

#include "tcp_connection_manager.h"
#include "tcp_reliability.h"
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace tcp_stack {

class TCPSocket {
public:
    TCPSocket();
    ~TCPSocket();
    
    // Non-copyable but movable
    TCPSocket(const TCPSocket&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;
    TCPSocket(TCPSocket&&) noexcept;
    TCPSocket& operator=(TCPSocket&&) noexcept;
    
    // Socket operations
    bool bind(const std::string& ip_address, uint16_t port);
    bool listen(int backlog = 5);
    std::unique_ptr<TCPSocket> accept();
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
    bool set_send_timeout(std::chrono::milliseconds timeout);
    
    // Get socket information
    std::string get_local_address() const;
    uint16_t get_local_port() const;
    std::string get_remote_address() const;
    uint16_t get_remote_port() const;
    
private:
    // Internal constructor for accepted connections
    TCPSocket(std::shared_ptr<TCPConnection> conn, 
             std::shared_ptr<TCPConnectionManager> manager);
    
    std::shared_ptr<TCPConnection> connection_;
    std::shared_ptr<TCPConnectionManager> connection_manager_;
    std::unique_ptr<TCPReliability> reliability_;
    
    // Receive buffer
    std::vector<uint8_t> receive_buffer_;
    std::mutex receive_mutex_;
    std::condition_variable receive_cv_;
    
    // Socket state
    bool is_listening_;
    bool is_blocking_;
    std::chrono::milliseconds recv_timeout_;
    std::chrono::milliseconds send_timeout_;
    
    // Bound address information
    uint32_t local_ip_;
    uint16_t local_port_;
    
    // Background thread for packet processing
    std::thread packet_processor_;
    std::atomic<bool> should_stop_;
    
    // Background packet processing
    void packet_processing_loop();
    void process_received_data(const std::vector<uint8_t>& data);
    
    // Helper methods
    uint32_t resolve_ip_address(const std::string& ip_str);
    bool start_packet_processor();
    void stop_packet_processor();
};

} // namespace tcp_stack