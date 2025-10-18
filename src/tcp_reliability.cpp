#include "tcp_reliability.h"
#include <algorithm>
#include <iostream>

namespace tcp_stack {

TCPReliability::TCPReliability()
    : next_seq_num_(0), last_ack_received_(0), rto_(std::chrono::milliseconds(1000)),
      srtt_(std::chrono::milliseconds(0)), rttvar_(std::chrono::milliseconds(0)),
      max_retransmits_(3), send_window_size_(65535), remote_window_size_(65535),
      bytes_in_flight_(0) {}

void TCPReliability::process_ack(uint32_t ack_num) {
    if (ack_num > last_ack_received_) {
        uint32_t newly_acked_bytes = ack_num - last_ack_received_;
        last_ack_received_ = ack_num;
        
        // Remove acknowledged segments
        remove_acknowledged_segments(ack_num);
        
        // Update bytes in flight
        if (bytes_in_flight_ >= newly_acked_bytes) {
            bytes_in_flight_ -= newly_acked_bytes;
        } else {
            bytes_in_flight_ = 0;
        }
        
        std::cout << "ACK received for seq " << ack_num 
                 << ", bytes in flight: " << bytes_in_flight_ << std::endl;
    }
}

bool TCPReliability::is_seq_acknowledged(uint32_t seq_num) const {
    return seq_num < last_ack_received_;
}

bool TCPReliability::can_send_data(size_t data_size) const {
    uint16_t effective_window = get_effective_window();
    return (bytes_in_flight_ + data_size) <= effective_window;
}

void TCPReliability::buffer_data(const std::vector<uint8_t>& data) {
    for (uint8_t byte : data) {
        send_buffer_.push(byte);
    }
}

std::vector<uint8_t> TCPReliability::get_data_to_send(size_t max_size) {
    std::vector<uint8_t> data;
    size_t available_window = get_effective_window() - bytes_in_flight_;
    size_t to_send = std::min({max_size, available_window, send_buffer_.size()});
    
    data.reserve(to_send);
    for (size_t i = 0; i < to_send && !send_buffer_.empty(); ++i) {
        data.push_back(send_buffer_.front());
        send_buffer_.pop();
    }
    
    if (!data.empty()) {
        // Create segment for tracking
        auto segment = std::make_shared<TCPSegment>(next_seq_num_, data);
        unacked_segments_.push_back(segment);
        
        // Update sequence number and bytes in flight
        advance_seq(data.size());
        bytes_in_flight_ += data.size();
    }
    
    return data;
}

std::vector<std::shared_ptr<TCPSegment>> TCPReliability::get_segments_to_retransmit() {
    std::vector<std::shared_ptr<TCPSegment>> segments_to_retx;
    auto now = std::chrono::steady_clock::now();
    
    for (auto& segment : unacked_segments_) {
        if (!segment->acknowledged && 
            (now - segment->sent_time) > rto_ &&
            segment->retransmit_count < max_retransmits_) {
            
            segments_to_retx.push_back(segment);
        }
    }
    
    return segments_to_retx;
}

void TCPReliability::mark_segment_sent(std::shared_ptr<TCPSegment> segment) {
    segment->sent_time = std::chrono::steady_clock::now();
    segment->retransmit_count++;
}

bool TCPReliability::has_timeout() const {
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& segment : unacked_segments_) {
        if (!segment->acknowledged &&
            (now - segment->sent_time) > rto_ &&
            segment->retransmit_count >= max_retransmits_) {
            return true;
        }
    }
    
    return false;
}

void TCPReliability::update_rtt(std::chrono::milliseconds rtt) {
    if (srtt_.count() == 0) {
        // First RTT measurement
        srtt_ = rtt;
        rttvar_ = rtt / 2;
    } else {
        // RFC 6298 RTT estimation
        auto rtt_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double, std::milli>(std::abs(srtt_.count() - rtt.count())));
        
        rttvar_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double, std::milli>(
                (1.0 - RTT_BETA) * rttvar_.count() + RTT_BETA * rtt_diff.count()));
        
        srtt_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double, std::milli>(
                (1.0 - RTT_ALPHA) * srtt_.count() + RTT_ALPHA * rtt.count()));
    }
    
    calculate_rto();
    
    std::cout << "RTT updated: " << rtt.count() << "ms, SRTT: " << srtt_.count() 
              << "ms, RTO: " << rto_.count() << "ms" << std::endl;
}

uint16_t TCPReliability::get_effective_window() const {
    return std::min(send_window_size_, remote_window_size_);
}

void TCPReliability::remove_acknowledged_segments(uint32_t ack_num) {
    auto it = std::remove_if(unacked_segments_.begin(), unacked_segments_.end(),
        [ack_num](const std::shared_ptr<TCPSegment>& segment) {
            if (segment->seq_num + segment->data.size() <= ack_num) {
                segment->acknowledged = true;
                return true;
            }
            return false;
        });
    
    unacked_segments_.erase(it, unacked_segments_.end());
}

void TCPReliability::calculate_rto() {
    // RFC 6298: RTO = SRTT + max(G, K * RTTVAR)
    auto k_rttvar = std::chrono::milliseconds(RTT_K * rttvar_.count());
    auto max_term = std::max(std::chrono::milliseconds(RTT_G), k_rttvar);
    
    rto_ = srtt_ + max_term;
    
    // Clamp RTO to reasonable bounds
    rto_ = std::max(rto_, std::chrono::milliseconds(200));   // Minimum 200ms
    rto_ = std::min(rto_, std::chrono::milliseconds(60000)); // Maximum 60s
}

} // namespace tcp_stack