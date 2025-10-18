#pragma once

#include <cstdint>
#include <arpa/inet.h>

namespace tcp_stack {

// TCP Header structure (RFC 793)
struct __attribute__((packed)) TCPHeader {
    uint16_t src_port;        // Source Port
    uint16_t dst_port;        // Destination Port
    uint32_t seq_num;         // Sequence Number
    uint32_t ack_num;         // Acknowledgment Number
    uint8_t data_offset_reserved; // Data Offset (4 bits) + Reserved (3 bits) + NS flag (1 bit)
    uint8_t flags;            // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t window_size;     // Window Size
    uint16_t checksum;        // Checksum
    uint16_t urgent_pointer;  // Urgent Pointer
    
    // TCP Flags
    static constexpr uint8_t FIN = 0x01;
    static constexpr uint8_t SYN = 0x02;
    static constexpr uint8_t RST = 0x04;
    static constexpr uint8_t PSH = 0x08;
    static constexpr uint8_t ACK = 0x10;
    static constexpr uint8_t URG = 0x20;
    static constexpr uint8_t ECE = 0x40;
    static constexpr uint8_t CWR = 0x80;
    
    // Helper methods
    uint8_t get_data_offset() const { return (data_offset_reserved >> 4) & 0xF; }
    uint8_t get_header_length() const { return get_data_offset() * 4; }
    
    void set_data_offset(uint8_t offset) {
        data_offset_reserved = (data_offset_reserved & 0x0F) | ((offset & 0x0F) << 4);
    }
    
    bool has_flag(uint8_t flag) const { return (flags & flag) != 0; }
    void set_flag(uint8_t flag) { flags |= flag; }
    void clear_flag(uint8_t flag) { flags &= ~flag; }
    void set_flags(uint8_t new_flags) { flags = new_flags; }
    
    // Convert to network byte order
    void to_network_order() {
        src_port = htons(src_port);
        dst_port = htons(dst_port);
        seq_num = htonl(seq_num);
        ack_num = htonl(ack_num);
        window_size = htons(window_size);
        checksum = htons(checksum);
        urgent_pointer = htons(urgent_pointer);
    }
    
    // Convert from network byte order
    void to_host_order() {
        src_port = ntohs(src_port);
        dst_port = ntohs(dst_port);
        seq_num = ntohl(seq_num);
        ack_num = ntohl(ack_num);
        window_size = ntohs(window_size);
        checksum = ntohs(checksum);
        urgent_pointer = ntohs(urgent_pointer);
    }
};

static_assert(sizeof(TCPHeader) == 20, "TCP header must be 20 bytes");

// TCP Pseudo Header for checksum calculation
struct __attribute__((packed)) TCPPseudoHeader {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
};

static_assert(sizeof(TCPPseudoHeader) == 12, "TCP pseudo header must be 12 bytes");

} // namespace tcp_stack