#pragma once

#include <cstdint>
#include <string>

namespace tcp_stack {

// TCP Connection States (RFC 793)
enum class TCPState {
    CLOSED,         // No connection state
    LISTEN,         // Waiting for connection request
    SYN_SENT,       // Waiting for matching connection request after sending connection request
    SYN_RECEIVED,   // Waiting for confirming connection request acknowledgment
    ESTABLISHED,    // Connection established, data transfer state
    FIN_WAIT_1,     // Waiting for connection termination request or acknowledgment
    FIN_WAIT_2,     // Waiting for connection termination request
    CLOSE_WAIT,     // Waiting for connection termination request from local user
    CLOSING,        // Waiting for connection termination request acknowledgment
    LAST_ACK,       // Waiting for connection termination request acknowledgment
    TIME_WAIT       // Waiting for enough time to ensure remote TCP received acknowledgment
};

// TCP Events that can trigger state transitions
enum class TCPEvent {
    PASSIVE_OPEN,   // Application calls listen()
    ACTIVE_OPEN,    // Application calls connect()
    SYN_RECEIVED,   // SYN segment received
    SYN_ACK_RECEIVED, // SYN+ACK segment received
    ACK_RECEIVED,   // ACK segment received (handshake completion)
    FIN_RECEIVED,   // FIN segment received
    CLOSE,          // Application calls close()
    TIMEOUT,        // Timer expiration
    RST_RECEIVED    // RST segment received
};

class TCPStateMachine {
public:
    TCPStateMachine();
    
    // Get current state
    TCPState get_state() const { return current_state_; }
    
    // Process an event and transition to new state
    TCPState process_event(TCPEvent event);
    
    // Check if connection is established
    bool is_established() const { return current_state_ == TCPState::ESTABLISHED; }
    
    // Check if connection is closed
    bool is_closed() const { return current_state_ == TCPState::CLOSED; }
    
    // Check if we can send data
    bool can_send_data() const;
    
    // Check if we can receive data
    bool can_receive_data() const;
    
    // Get state name as string
    std::string get_state_name() const;
    
    // Get state name for any state
    std::string get_state_name_for_state(TCPState state) const;
    
    // Reset to initial state
    void reset() { current_state_ = TCPState::CLOSED; }
    
private:
    TCPState current_state_;
    
    // State transition table
    TCPState transition(TCPState current, TCPEvent event);
};

} // namespace tcp_stack