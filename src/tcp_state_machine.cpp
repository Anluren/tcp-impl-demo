#include "tcp_state_machine.h"
#include <iostream>

namespace tcp_stack {

TCPStateMachine::TCPStateMachine() : current_state_(TCPState::CLOSED) {}

TCPState TCPStateMachine::process_event(TCPEvent event) {
    TCPState new_state = transition(current_state_, event);
    
    if (new_state != current_state_) {
        std::cout << "TCP State transition: " << get_state_name() 
                 << " -> " << get_state_name_for_state(new_state) << std::endl;
        current_state_ = new_state;
    }
    
    return current_state_;
}

bool TCPStateMachine::can_send_data() const {
    return current_state_ == TCPState::ESTABLISHED ||
           current_state_ == TCPState::CLOSE_WAIT;
}

bool TCPStateMachine::can_receive_data() const {
    return current_state_ == TCPState::ESTABLISHED ||
           current_state_ == TCPState::FIN_WAIT_1 ||
           current_state_ == TCPState::FIN_WAIT_2;
}

std::string TCPStateMachine::get_state_name() const {
    return get_state_name_for_state(current_state_);
}

std::string TCPStateMachine::get_state_name_for_state(TCPState state) const {
    switch (state) {
        case TCPState::CLOSED:       return "CLOSED";
        case TCPState::LISTEN:       return "LISTEN";
        case TCPState::SYN_SENT:     return "SYN_SENT";
        case TCPState::SYN_RECEIVED: return "SYN_RECEIVED";
        case TCPState::ESTABLISHED:  return "ESTABLISHED";
        case TCPState::FIN_WAIT_1:   return "FIN_WAIT_1";
        case TCPState::FIN_WAIT_2:   return "FIN_WAIT_2";
        case TCPState::CLOSE_WAIT:   return "CLOSE_WAIT";
        case TCPState::CLOSING:      return "CLOSING";
        case TCPState::LAST_ACK:     return "LAST_ACK";
        case TCPState::TIME_WAIT:    return "TIME_WAIT";
        default:                     return "UNKNOWN";
    }
}

TCPState TCPStateMachine::transition(TCPState current, TCPEvent event) {
    switch (current) {
        case TCPState::CLOSED:
            switch (event) {
                case TCPEvent::PASSIVE_OPEN:
                    return TCPState::LISTEN;
                case TCPEvent::ACTIVE_OPEN:
                    return TCPState::SYN_SENT;
                default:
                    return current;
            }
            
        case TCPState::LISTEN:
            switch (event) {
                case TCPEvent::SYN_RECEIVED:
                    return TCPState::SYN_RECEIVED;
                case TCPEvent::CLOSE:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::SYN_SENT:
            switch (event) {
                case TCPEvent::SYN_ACK_RECEIVED:
                    return TCPState::ESTABLISHED;
                case TCPEvent::SYN_RECEIVED:
                    return TCPState::SYN_RECEIVED;
                case TCPEvent::CLOSE:
                case TCPEvent::TIMEOUT:
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::SYN_RECEIVED:
            switch (event) {
                case TCPEvent::ACK_RECEIVED:
                    return TCPState::ESTABLISHED;
                case TCPEvent::CLOSE:
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::ESTABLISHED:
            switch (event) {
                case TCPEvent::CLOSE:
                    return TCPState::FIN_WAIT_1;
                case TCPEvent::FIN_RECEIVED:
                    return TCPState::CLOSE_WAIT;
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::FIN_WAIT_1:
            switch (event) {
                case TCPEvent::ACK_RECEIVED:
                    return TCPState::FIN_WAIT_2;
                case TCPEvent::FIN_RECEIVED:
                    return TCPState::CLOSING;
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::FIN_WAIT_2:
            switch (event) {
                case TCPEvent::FIN_RECEIVED:
                    return TCPState::TIME_WAIT;
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::CLOSE_WAIT:
            switch (event) {
                case TCPEvent::CLOSE:
                    return TCPState::LAST_ACK;
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::CLOSING:
            switch (event) {
                case TCPEvent::ACK_RECEIVED:
                    return TCPState::TIME_WAIT;
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::LAST_ACK:
            switch (event) {
                case TCPEvent::ACK_RECEIVED:
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        case TCPState::TIME_WAIT:
            switch (event) {
                case TCPEvent::TIMEOUT:
                case TCPEvent::RST_RECEIVED:
                    return TCPState::CLOSED;
                default:
                    return current;
            }
            
        default:
            return current;
    }
}

} // namespace tcp_stack