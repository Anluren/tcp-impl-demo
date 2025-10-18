#include "ip_layer.h"
#include "network_utils.h"
#include <arpa/inet.h>
#include <cstring>
#include <random>
#include <iostream>

namespace tcp_stack {

IPLayer::IPLayer() : packet_id_(1) {
    raw_socket_ = std::make_unique<RawSocket>();
}

bool IPLayer::initialize() {
    return raw_socket_->initialize();
}

std::vector<uint8_t> IPLayer::create_packet(uint32_t src_ip, uint32_t dst_ip, 
                                           uint8_t protocol, const std::vector<uint8_t>& payload) {
    IPHeader ip_header = create_ip_header(src_ip, dst_ip, protocol, payload.size());
    
    // Calculate and set checksum
    ip_header.checksum = 0;
    ip_header.checksum = htons(calculate_checksum(ip_header));
    
    // Create packet
    std::vector<uint8_t> packet(sizeof(IPHeader) + payload.size());
    
    // Copy header
    std::memcpy(packet.data(), &ip_header, sizeof(IPHeader));
    
    // Copy payload
    if (!payload.empty()) {
        std::memcpy(packet.data() + sizeof(IPHeader), payload.data(), payload.size());
    }
    
    return packet;
}

bool IPLayer::parse_packet(const std::vector<uint8_t>& packet, IPHeader& ip_header, 
                          std::vector<uint8_t>& payload) {
    if (packet.size() < sizeof(IPHeader)) {
        return false;
    }
    
    // Extract header
    std::memcpy(&ip_header, packet.data(), sizeof(IPHeader));
    
    // Validate header
    if (ip_header.get_version() != 4) {
        return false; // Only IPv4 supported
    }
    
    uint16_t header_length = ip_header.get_header_length();
    if (header_length < sizeof(IPHeader) || header_length > packet.size()) {
        return false;
    }
    
    uint16_t total_length = ntohs(ip_header.total_length);
    if (total_length > packet.size() || total_length < header_length) {
        return false;
    }
    
    // Validate checksum
    if (!validate_checksum(ip_header)) {
        std::cerr << "IP header checksum validation failed" << std::endl;
        return false;
    }
    
    // Extract payload
    size_t payload_length = total_length - header_length;
    if (payload_length > 0) {
        payload.resize(payload_length);
        std::memcpy(payload.data(), packet.data() + header_length, payload_length);
    } else {
        payload.clear();
    }
    
    return true;
}

bool IPLayer::send_packet(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, 
                         const std::vector<uint8_t>& payload) {
    if (!raw_socket_->is_valid()) {
        return false;
    }
    
    auto packet = create_packet(src_ip, dst_ip, protocol, payload);
    return raw_socket_->send_packet(packet, dst_ip);
}

bool IPLayer::receive_packet(IPHeader& ip_header, std::vector<uint8_t>& payload) {
    if (!raw_socket_->is_valid()) {
        return false;
    }
    
    std::vector<uint8_t> packet;
    uint32_t src_ip;
    
    if (!raw_socket_->receive_packet(packet, src_ip)) {
        return false;
    }
    
    return parse_packet(packet, ip_header, payload);
}

bool IPLayer::validate_checksum(const IPHeader& header) {
    IPHeader temp_header = header;
    temp_header.checksum = 0;
    
    uint16_t calculated_checksum = calculate_checksum(temp_header);
    return ntohs(header.checksum) == calculated_checksum;
}

uint16_t IPLayer::calculate_checksum(const IPHeader& header) {
    return NetworkUtils::calculate_checksum(&header, sizeof(IPHeader));
}

IPHeader IPLayer::create_ip_header(uint32_t src_ip, uint32_t dst_ip, 
                                  uint8_t protocol, uint16_t payload_length) {
    IPHeader header;
    std::memset(&header, 0, sizeof(header));
    
    header.set_version(4);                    // IPv4
    header.set_ihl(5);                        // 20 bytes (no options)
    header.tos = 0;                           // Default type of service
    header.total_length = htons(sizeof(IPHeader) + payload_length);
    header.identification = htons(packet_id_++);
    header.set_flags_fragment(0x2, 0);        // Don't Fragment flag set
    header.ttl = 64;                          // Default TTL
    header.protocol = protocol;
    header.checksum = 0;                      // Will be calculated later
    header.src_ip = src_ip;
    header.dst_ip = dst_ip;
    
    return header;
}

} // namespace tcp_stack