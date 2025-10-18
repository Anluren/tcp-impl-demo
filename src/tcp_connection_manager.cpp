#include "tcp_connection_manager.h"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace tcp_stack {

TCPConnectionManager::TCPConnectionManager() {
    ip_layer_ = std::make_unique<IPLayer>();
}

bool TCPConnectionManager::initialize() {
    return ip_layer_->initialize();
}

bool TCPConnectionManager::listen(uint32_t local_ip, uint16_t local_port) {
    auto listening_conn = std::make_shared<TCPConnection>();
    listening_conn->local_ip = local_ip;
    listening_conn->local_port = local_port;
    listening_conn->remote_ip = 0;
    listening_conn->remote_port = 0;
    listening_conn->state_machine.process_event(TCPEvent::PASSIVE_OPEN);
    listening_conn->last_activity = std::chrono::steady_clock::now();
    
    listening_sockets_.push_back(listening_conn);
    std::cout << "Listening on " << NetworkUtils::ip_network_to_string(local_ip) 
              << ":" << local_port << std::endl;
    return true;
}

std::shared_ptr<TCPConnection> TCPConnectionManager::accept_connection() {
    // Process incoming packets to handle SYN requests
    IPHeader ip_header;
    std::vector<uint8_t> payload;
    
    while (ip_layer_->receive_packet(ip_header, payload)) {
        if (ip_header.protocol == IPPROTO_TCP) {
            process_incoming_segment(ip_header, payload);
        }
    }
    
    // Look for established connections
    for (auto& conn : connections_) {
        if (conn->state_machine.is_established()) {
            return conn;
        }
    }
    
    return nullptr;
}

std::shared_ptr<TCPConnection> TCPConnectionManager::connect(uint32_t local_ip, uint16_t local_port,
                                                           uint32_t remote_ip, uint16_t remote_port) {
    auto conn = std::make_shared<TCPConnection>();
    conn->local_ip = local_ip;
    conn->local_port = local_port;
    conn->remote_ip = remote_ip;
    conn->remote_port = remote_port;
    conn->local_seq = NetworkUtils::generate_sequence_number();
    conn->window_size = 65535; // Default window size
    conn->last_activity = std::chrono::steady_clock::now();
    
    connections_.push_back(conn);
    
    // Initiate connection with SYN
    conn->state_machine.process_event(TCPEvent::ACTIVE_OPEN);
    if (!send_syn(conn)) {
        remove_connection(conn);
        return nullptr;
    }
    
    return conn;
}

bool TCPConnectionManager::send_segment(std::shared_ptr<TCPConnection> conn, 
                                       const std::vector<uint8_t>& data, uint8_t flags) {
    if (!conn || !conn->state_machine.can_send_data()) {
        return false;
    }
    
    TCPHeader tcp_header = create_tcp_header(conn, data, flags);
    
    // Create TCP segment
    std::vector<uint8_t> tcp_segment(sizeof(TCPHeader) + data.size());
    
    // Copy header (convert to network byte order)
    TCPHeader net_header = tcp_header;
    net_header.to_network_order();
    std::memcpy(tcp_segment.data(), &net_header, sizeof(TCPHeader));
    
    // Copy data
    if (!data.empty()) {
        std::memcpy(tcp_segment.data() + sizeof(TCPHeader), data.data(), data.size());
    }
    
    // Send via IP layer
    bool success = ip_layer_->send_packet(conn->local_ip, conn->remote_ip, 
                                         IPPROTO_TCP, tcp_segment);
    
    if (success && !data.empty()) {
        conn->local_seq += data.size();
    }
    
    conn->last_activity = std::chrono::steady_clock::now();
    return success;
}

bool TCPConnectionManager::process_incoming_segment(const IPHeader& ip_header, 
                                                   const std::vector<uint8_t>& tcp_data) {
    if (tcp_data.size() < sizeof(TCPHeader)) {
        return false;
    }
    
    TCPHeader tcp_header;
    std::memcpy(&tcp_header, tcp_data.data(), sizeof(TCPHeader));
    tcp_header.to_host_order();
    
    // Validate checksum
    uint16_t received_checksum = tcp_header.checksum;
    tcp_header.checksum = 0;
    uint16_t calculated_checksum = calculate_tcp_checksum(ip_header.src_ip, ip_header.dst_ip,
                                                         tcp_header, 
                                                         std::vector<uint8_t>(tcp_data.begin() + sizeof(TCPHeader), 
                                                                             tcp_data.end()));
    
    if (received_checksum != calculated_checksum) {
        std::cerr << "TCP checksum mismatch" << std::endl;
        return false;
    }
    
    // Extract data payload
    std::vector<uint8_t> data;
    if (tcp_data.size() > sizeof(TCPHeader)) {
        data.assign(tcp_data.begin() + sizeof(TCPHeader), tcp_data.end());
    }
    
    // Handle different segment types
    if (tcp_header.has_flag(TCPHeader::SYN)) {
        if (tcp_header.has_flag(TCPHeader::ACK)) {
            handle_syn_ack_segment(ip_header, tcp_header);
        } else {
            handle_syn_segment(ip_header, tcp_header);
        }
    } else if (tcp_header.has_flag(TCPHeader::ACK)) {
        handle_ack_segment(ip_header, tcp_header);
    } else if (tcp_header.has_flag(TCPHeader::FIN)) {
        handle_fin_segment(ip_header, tcp_header);
    } else if (tcp_header.has_flag(TCPHeader::RST)) {
        handle_rst_segment(ip_header, tcp_header);
    }
    
    if (!data.empty()) {
        handle_data_segment(ip_header, tcp_header, data);
    }
    
    return true;
}

bool TCPConnectionManager::close_connection(std::shared_ptr<TCPConnection> conn) {
    if (!conn) return false;
    
    conn->state_machine.process_event(TCPEvent::CLOSE);
    bool success = send_fin(conn);
    
    // Remove from connections list after close
    remove_connection(conn);
    return success;
}

std::shared_ptr<TCPConnection> TCPConnectionManager::find_connection(uint32_t local_ip, uint16_t local_port,
                                                                    uint32_t remote_ip, uint16_t remote_port) {
    auto it = std::find_if(connections_.begin(), connections_.end(),
        [=](const std::shared_ptr<TCPConnection>& conn) {
            return conn->local_ip == local_ip && conn->local_port == local_port &&
                   conn->remote_ip == remote_ip && conn->remote_port == remote_port;
        });
    
    return (it != connections_.end()) ? *it : nullptr;
}

TCPHeader TCPConnectionManager::create_tcp_header(std::shared_ptr<TCPConnection> conn,
                                                 const std::vector<uint8_t>& data, uint8_t flags) {
    TCPHeader header;
    std::memset(&header, 0, sizeof(header));
    
    header.src_port = conn->local_port;
    header.dst_port = conn->remote_port;
    header.seq_num = conn->local_seq;
    header.ack_num = conn->local_ack;
    header.set_data_offset(5); // 20 bytes, no options
    header.flags = flags;
    header.window_size = conn->window_size;
    header.urgent_pointer = 0;
    
    // Calculate checksum
    header.checksum = 0;
    header.checksum = calculate_tcp_checksum(conn->local_ip, conn->remote_ip, header, data);
    
    return header;
}

uint16_t TCPConnectionManager::calculate_tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                                                     const TCPHeader& header, 
                                                     const std::vector<uint8_t>& data) {
    // Create pseudo header for checksum calculation
    TCPPseudoHeader pseudo_header;
    pseudo_header.src_ip = src_ip;
    pseudo_header.dst_ip = dst_ip;
    pseudo_header.reserved = 0;
    pseudo_header.protocol = IPPROTO_TCP;
    pseudo_header.tcp_length = htons(sizeof(TCPHeader) + data.size());
    
    // Prepare segments for checksum calculation
    std::vector<std::pair<const void*, size_t>> segments = {
        {&pseudo_header, sizeof(pseudo_header)},
        {&header, sizeof(header)}
    };
    
    if (!data.empty()) {
        segments.push_back({data.data(), data.size()});
    }
    
    return NetworkUtils::calculate_checksum(segments);
}

// Handle different segment types
void TCPConnectionManager::handle_syn_segment(const IPHeader& ip_header, const TCPHeader& tcp_header) {
    // Look for listening socket
    auto listening_it = std::find_if(listening_sockets_.begin(), listening_sockets_.end(),
        [&](const std::shared_ptr<TCPConnection>& conn) {
            return conn->local_ip == ip_header.dst_ip && conn->local_port == tcp_header.dst_port;
        });
    
    if (listening_it != listening_sockets_.end()) {
        // Create new connection
        auto new_conn = std::make_shared<TCPConnection>();
        new_conn->local_ip = ip_header.dst_ip;
        new_conn->local_port = tcp_header.dst_port;
        new_conn->remote_ip = ip_header.src_ip;
        new_conn->remote_port = tcp_header.src_port;
        new_conn->remote_seq = tcp_header.seq_num;
        new_conn->local_ack = tcp_header.seq_num + 1;
        new_conn->local_seq = NetworkUtils::generate_sequence_number();
        new_conn->window_size = 65535;
        new_conn->last_activity = std::chrono::steady_clock::now();
        
        new_conn->state_machine.process_event(TCPEvent::SYN_RECEIVED);
        connections_.push_back(new_conn);
        
        // Send SYN-ACK
        send_syn_ack(new_conn);
    }
}

void TCPConnectionManager::handle_syn_ack_segment(const IPHeader& ip_header, const TCPHeader& tcp_header) {
    auto conn = find_connection(ip_header.dst_ip, tcp_header.dst_port,
                               ip_header.src_ip, tcp_header.src_port);
    if (conn) {
        conn->remote_seq = tcp_header.seq_num;
        conn->local_ack = tcp_header.seq_num + 1;
        conn->state_machine.process_event(TCPEvent::SYN_ACK_RECEIVED);
        conn->last_activity = std::chrono::steady_clock::now();
        
        // Send ACK to complete handshake
        send_ack(conn);
    }
}

void TCPConnectionManager::handle_ack_segment(const IPHeader& ip_header, const TCPHeader& tcp_header) {
    auto conn = find_connection(ip_header.dst_ip, tcp_header.dst_port,
                               ip_header.src_ip, tcp_header.src_port);
    if (conn) {
        conn->state_machine.process_event(TCPEvent::ACK_RECEIVED);
        conn->last_activity = std::chrono::steady_clock::now();
    }
}

void TCPConnectionManager::handle_fin_segment(const IPHeader& ip_header, const TCPHeader& tcp_header) {
    auto conn = find_connection(ip_header.dst_ip, tcp_header.dst_port,
                               ip_header.src_ip, tcp_header.src_port);
    if (conn) {
        conn->state_machine.process_event(TCPEvent::FIN_RECEIVED);
        conn->local_ack = tcp_header.seq_num + 1;
        conn->last_activity = std::chrono::steady_clock::now();
        
        // Send ACK for FIN
        send_ack(conn);
    }
}

void TCPConnectionManager::handle_rst_segment(const IPHeader& ip_header, const TCPHeader& tcp_header) {
    auto conn = find_connection(ip_header.dst_ip, tcp_header.dst_port,
                               ip_header.src_ip, tcp_header.src_port);
    if (conn) {
        conn->state_machine.process_event(TCPEvent::RST_RECEIVED);
        remove_connection(conn);
    }
}

void TCPConnectionManager::handle_data_segment(const IPHeader& ip_header, const TCPHeader& tcp_header,
                                              const std::vector<uint8_t>& data) {
    auto conn = find_connection(ip_header.dst_ip, tcp_header.dst_port,
                               ip_header.src_ip, tcp_header.src_port);
    if (conn && conn->state_machine.can_receive_data()) {
        conn->local_ack = tcp_header.seq_num + data.size();
        conn->last_activity = std::chrono::steady_clock::now();
        
        // Send ACK for received data
        send_ack(conn);
        
        // TODO: Buffer data for application
        std::cout << "Received " << data.size() << " bytes of data" << std::endl;
    }
}

// Send specific TCP segments
bool TCPConnectionManager::send_syn(std::shared_ptr<TCPConnection> conn) {
    return send_segment(conn, {}, TCPHeader::SYN);
}

bool TCPConnectionManager::send_syn_ack(std::shared_ptr<TCPConnection> conn) {
    return send_segment(conn, {}, TCPHeader::SYN | TCPHeader::ACK);
}

bool TCPConnectionManager::send_ack(std::shared_ptr<TCPConnection> conn) {
    return send_segment(conn, {}, TCPHeader::ACK);
}

bool TCPConnectionManager::send_fin(std::shared_ptr<TCPConnection> conn) {
    return send_segment(conn, {}, TCPHeader::FIN | TCPHeader::ACK);
}

bool TCPConnectionManager::send_rst(std::shared_ptr<TCPConnection> conn) {
    return send_segment(conn, {}, TCPHeader::RST);
}

void TCPConnectionManager::remove_connection(std::shared_ptr<TCPConnection> conn) {
    connections_.erase(
        std::remove(connections_.begin(), connections_.end(), conn),
        connections_.end());
}

} // namespace tcp_stack