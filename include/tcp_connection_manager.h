#pragma once

#include "tcp_header.h"
#include "tcp_state_machine.h"
#include "ip_layer.h"
#include "network_utils.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>

namespace tcp_stack {

struct TCPConnection {
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    
    uint32_t local_seq;     // Our sequence number
    uint32_t remote_seq;    // Remote sequence number
    uint32_t local_ack;     // Our acknowledgment number
    uint16_t window_size;   // Our receive window size
    
    TCPStateMachine state_machine;
    std::chrono::steady_clock::time_point last_activity;
    
    bool operator==(const TCPConnection& other) const {
        return local_ip == other.local_ip && local_port == other.local_port &&
               remote_ip == other.remote_ip && remote_port == other.remote_port;
    }
};

class TCPConnectionManager {
public:
    TCPConnectionManager();
    ~TCPConnectionManager() = default;
    
    // Initialize the connection manager
    bool initialize();
    
    // Server-side operations
    bool listen(uint32_t local_ip, uint16_t local_port);
    std::shared_ptr<TCPConnection> accept_connection();
    
    // Client-side operations
    std::shared_ptr<TCPConnection> connect(uint32_t local_ip, uint16_t local_port,
                                          uint32_t remote_ip, uint16_t remote_port);
    
    // Send TCP segment
    bool send_segment(std::shared_ptr<TCPConnection> conn, const std::vector<uint8_t>& data,
                     uint8_t flags = 0);
    
    // Process incoming TCP segment
    bool process_incoming_segment(const IPHeader& ip_header, const std::vector<uint8_t>& tcp_data);
    
    // Close connection
    bool close_connection(std::shared_ptr<TCPConnection> conn);
    
    // Get connection by 4-tuple
    std::shared_ptr<TCPConnection> find_connection(uint32_t local_ip, uint16_t local_port,
                                                  uint32_t remote_ip, uint16_t remote_port);
    
private:
    std::unique_ptr<IPLayer> ip_layer_;
    std::vector<std::shared_ptr<TCPConnection>> connections_;
    std::vector<std::shared_ptr<TCPConnection>> listening_sockets_;
    
    // Create TCP header for a connection
    TCPHeader create_tcp_header(std::shared_ptr<TCPConnection> conn, 
                               const std::vector<uint8_t>& data, uint8_t flags);
    
    // Calculate TCP checksum
    uint16_t calculate_tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                                   const TCPHeader& header, const std::vector<uint8_t>& data);
    
    // Handle different TCP segments
    void handle_syn_segment(const IPHeader& ip_header, const TCPHeader& tcp_header);
    void handle_syn_ack_segment(const IPHeader& ip_header, const TCPHeader& tcp_header);
    void handle_ack_segment(const IPHeader& ip_header, const TCPHeader& tcp_header);
    void handle_fin_segment(const IPHeader& ip_header, const TCPHeader& tcp_header);
    void handle_rst_segment(const IPHeader& ip_header, const TCPHeader& tcp_header);
    void handle_data_segment(const IPHeader& ip_header, const TCPHeader& tcp_header,
                           const std::vector<uint8_t>& data);
    
    // Send specific TCP segments
    bool send_syn(std::shared_ptr<TCPConnection> conn);
    bool send_syn_ack(std::shared_ptr<TCPConnection> conn);
    bool send_ack(std::shared_ptr<TCPConnection> conn);
    bool send_fin(std::shared_ptr<TCPConnection> conn);
    bool send_rst(std::shared_ptr<TCPConnection> conn);
    
    // Remove connection from list
    void remove_connection(std::shared_ptr<TCPConnection> conn);
};

} // namespace tcp_stack