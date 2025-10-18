#pragma once

#include "ip_header.h"
#include "raw_socket.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace tcp_stack {

class IPLayer {
public:
    IPLayer();
    ~IPLayer() = default;
    
    // Initialize the IP layer
    bool initialize();
    
    // Create an IP packet with the given payload
    std::vector<uint8_t> create_packet(uint32_t src_ip, uint32_t dst_ip, 
                                      uint8_t protocol, const std::vector<uint8_t>& payload);
    
    // Parse an IP packet and extract payload
    bool parse_packet(const std::vector<uint8_t>& packet, IPHeader& ip_header, 
                     std::vector<uint8_t>& payload);
    
    // Send an IP packet
    bool send_packet(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, 
                    const std::vector<uint8_t>& payload);
    
    // Receive an IP packet (non-blocking)
    bool receive_packet(IPHeader& ip_header, std::vector<uint8_t>& payload);
    
    // Validate IP header checksum
    bool validate_checksum(const IPHeader& header);
    
    // Calculate IP header checksum
    uint16_t calculate_checksum(const IPHeader& header);
    
private:
    std::unique_ptr<RawSocket> raw_socket_;
    uint16_t packet_id_;
    
    // Create IP header
    IPHeader create_ip_header(uint32_t src_ip, uint32_t dst_ip, 
                             uint8_t protocol, uint16_t payload_length);
};

} // namespace tcp_stack