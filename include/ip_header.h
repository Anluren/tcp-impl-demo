#pragma once

#include <cstdint>
#include <arpa/inet.h>

namespace tcp_stack {

// IP Protocol numbers
constexpr uint8_t IPPROTO_TCP = 6;

// IP Header structure (RFC 791)
struct __attribute__((packed)) IPHeader {
    uint8_t version_ihl;      // Version (4 bits) + IHL (4 bits)
    uint8_t tos;              // Type of Service
    uint16_t total_length;    // Total Length
    uint16_t identification;  // Identification
    uint16_t flags_fragment;  // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t ttl;              // Time to Live
    uint8_t protocol;         // Protocol
    uint16_t checksum;        // Header Checksum
    uint32_t src_ip;          // Source Address
    uint32_t dst_ip;          // Destination Address
    
    // Helper methods
    uint8_t get_version() const { return (version_ihl >> 4) & 0xF; }
    uint8_t get_ihl() const { return version_ihl & 0xF; }
    uint8_t get_header_length() const { return get_ihl() * 4; }
    
    void set_version(uint8_t version) {
        version_ihl = (version_ihl & 0x0F) | ((version & 0x0F) << 4);
    }
    
    void set_ihl(uint8_t ihl) {
        version_ihl = (version_ihl & 0xF0) | (ihl & 0x0F);
    }
    
    uint16_t get_flags() const { return ntohs(flags_fragment) >> 13; }
    uint16_t get_fragment_offset() const { return ntohs(flags_fragment) & 0x1FFF; }
    
    void set_flags_fragment(uint16_t flags, uint16_t fragment_offset) {
        flags_fragment = htons(((flags & 0x7) << 13) | (fragment_offset & 0x1FFF));
    }
};

static_assert(sizeof(IPHeader) == 20, "IP header must be 20 bytes");

} // namespace tcp_stack