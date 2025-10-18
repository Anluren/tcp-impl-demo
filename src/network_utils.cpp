#include "network_utils.h"
#include <arpa/inet.h>
#include <random>
#include <chrono>

namespace tcp_stack {

uint16_t NetworkUtils::calculate_checksum(const void* data, size_t length) {
    uint32_t sum = checksum_accumulate(data, length);
    
    // Add carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // One's complement
    return static_cast<uint16_t>(~sum);
}

uint16_t NetworkUtils::calculate_checksum(const std::vector<std::pair<const void*, size_t>>& segments) {
    uint32_t sum = 0;
    
    for (const auto& segment : segments) {
        sum = checksum_accumulate(segment.first, segment.second, sum);
    }
    
    // Add carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // One's complement
    return static_cast<uint16_t>(~sum);
}

uint32_t NetworkUtils::ip_string_to_network(const std::string& ip_str) {
    struct in_addr addr;
    if (inet_aton(ip_str.c_str(), &addr) == 0) {
        return 0; // Invalid IP
    }
    return addr.s_addr;
}

std::string NetworkUtils::ip_network_to_string(uint32_t ip_addr) {
    struct in_addr addr;
    addr.s_addr = ip_addr;
    return std::string(inet_ntoa(addr));
}

uint32_t NetworkUtils::generate_sequence_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
    return dis(gen);
}

uint32_t NetworkUtils::checksum_accumulate(const void* data, size_t length, uint32_t sum) {
    const uint16_t* ptr = static_cast<const uint16_t*>(data);
    
    // Process 16-bit words
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    // Handle odd byte
    if (length == 1) {
        sum += *reinterpret_cast<const uint8_t*>(ptr) << 8;
    }
    
    return sum;
}

} // namespace tcp_stack