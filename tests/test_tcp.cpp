#include "tcp_socket.h"
#include "tcp_state_machine.h"
#include "network_utils.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace tcp_stack;

void test_state_machine() {
    std::cout << "Testing TCP State Machine..." << std::endl;
    
    TCPStateMachine sm;
    assert(sm.get_state() == TCPState::CLOSED);
    
    // Test passive open (server)
    sm.process_event(TCPEvent::PASSIVE_OPEN);
    assert(sm.get_state() == TCPState::LISTEN);
    
    sm.process_event(TCPEvent::SYN_RECEIVED);
    assert(sm.get_state() == TCPState::SYN_RECEIVED);
    
    sm.process_event(TCPEvent::ACK_RECEIVED);
    assert(sm.is_established());
    
    // Test connection close
    sm.process_event(TCPEvent::CLOSE);
    assert(sm.get_state() == TCPState::FIN_WAIT_1);
    
    std::cout << "State machine tests passed!" << std::endl;
}

void test_network_utils() {
    std::cout << "Testing Network Utils..." << std::endl;
    
    // Test IP conversion
    uint32_t ip = NetworkUtils::ip_string_to_network("192.168.1.1");
    std::string ip_str = NetworkUtils::ip_network_to_string(ip);
    assert(ip_str == "192.168.1.1");
    
    // Test checksum calculation
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    uint16_t checksum = NetworkUtils::calculate_checksum(data.data(), data.size());
    assert(checksum != 0); // Should not be zero for this data
    
    std::cout << "Network utils tests passed!" << std::endl;
}

void test_socket_creation() {
    std::cout << "Testing Socket Creation..." << std::endl;
    
    TCPSocket socket;
    assert(!socket.is_connected());
    
    // Test binding
    bool bind_result = socket.bind("127.0.0.1", 0); // Use port 0 for any available port
    std::cout << "Bind result: " << (bind_result ? "SUCCESS" : "FAILED") << std::endl;
    
    std::cout << "Socket creation tests completed!" << std::endl;
}

int main() {
    std::cout << "Running TCP Stack Tests..." << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        test_state_machine();
        test_network_utils();
        test_socket_creation();
        
        std::cout << "===========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        
        // Note: Full integration tests would require root privileges
        // for raw socket operations and would be more complex to set up
        std::cout << "\nNote: This is a basic test suite." << std::endl;
        std::cout << "Full integration testing requires root privileges" << std::endl;
        std::cout << "for raw socket operations." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}