#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>

namespace tcp_stack {

// Utility functions for network operations
class NetworkUtils {
public:
    // Calculate Internet checksum (RFC 1071)
    static uint16_t calculate_checksum(const void* data, size_t length);
    
    // Calculate checksum with multiple data segments
    static uint16_t calculate_checksum(const std::vector<std::pair<const void*, size_t>>& segments);
    
    // Convert IP address from string to network byte order
    static uint32_t ip_string_to_network(const std::string& ip_str);
    
    // Convert IP address from network byte order to string
    static std::string ip_network_to_string(uint32_t ip_addr);
    
    // Generate random sequence number
    static uint32_t generate_sequence_number();
    
private:
    static uint32_t checksum_accumulate(const void* data, size_t length, uint32_t sum = 0);
};

} // namespace tcp_stack