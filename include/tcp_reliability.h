#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <queue>
#include <memory>

namespace tcp_stack {

struct TCPSegment {
    uint32_t seq_num;
    std::vector<uint8_t> data;
    std::chrono::steady_clock::time_point sent_time;
    uint8_t retransmit_count;
    bool acknowledged;
    
    TCPSegment(uint32_t seq, const std::vector<uint8_t>& segment_data)
        : seq_num(seq), data(segment_data), sent_time(std::chrono::steady_clock::now()),
          retransmit_count(0), acknowledged(false) {}
};

class TCPReliability {
public:
    TCPReliability();
    
    // Configure parameters
    void set_initial_rto(std::chrono::milliseconds rto) { rto_ = rto; }
    void set_max_retransmits(uint8_t max_retx) { max_retransmits_ = max_retx; }
    void set_window_size(uint16_t window) { send_window_size_ = window; }
    
    // Sequence number management
    uint32_t get_next_seq() const { return next_seq_num_; }
    void advance_seq(uint32_t bytes) { next_seq_num_ += bytes; }
    void set_initial_seq(uint32_t seq) { next_seq_num_ = seq; }
    
    // Acknowledgment handling
    void process_ack(uint32_t ack_num);
    bool is_seq_acknowledged(uint32_t seq_num) const;
    
    // Send buffer management
    bool can_send_data(size_t data_size) const;
    void buffer_data(const std::vector<uint8_t>& data);
    std::vector<uint8_t> get_data_to_send(size_t max_size);
    
    // Retransmission handling
    std::vector<std::shared_ptr<TCPSegment>> get_segments_to_retransmit();
    void mark_segment_sent(std::shared_ptr<TCPSegment> segment);
    
    // Timeout management
    bool has_timeout() const;
    std::chrono::milliseconds get_rto() const { return rto_; }
    void update_rtt(std::chrono::milliseconds rtt);
    
    // Flow control
    void update_remote_window(uint16_t window) { remote_window_size_ = window; }
    uint16_t get_effective_window() const;
    
    // Statistics
    uint32_t get_bytes_in_flight() const { return bytes_in_flight_; }
    uint32_t get_last_ack() const { return last_ack_received_; }
    
private:
    // Sequence numbers
    uint32_t next_seq_num_;
    uint32_t last_ack_received_;
    
    // Buffers
    std::queue<uint8_t> send_buffer_;
    std::vector<std::shared_ptr<TCPSegment>> unacked_segments_;
    
    // Timing and retransmission
    std::chrono::milliseconds rto_;                    // Retransmission timeout
    std::chrono::milliseconds srtt_;                   // Smoothed RTT
    std::chrono::milliseconds rttvar_;                 // RTT variation
    uint8_t max_retransmits_;
    
    // Flow control
    uint16_t send_window_size_;                        // Our send window
    uint16_t remote_window_size_;                      // Remote's receive window
    uint32_t bytes_in_flight_;                         // Unacknowledged bytes
    
    // Constants for RTT calculation (RFC 6298)
    static constexpr double RTT_ALPHA = 0.125;
    static constexpr double RTT_BETA = 0.25;
    static constexpr int RTT_K = 4;
    static constexpr int RTT_G = 100; // Clock granularity in ms
    
    // Helper methods
    void remove_acknowledged_segments(uint32_t ack_num);
    void calculate_rto();
};

} // namespace tcp_stack