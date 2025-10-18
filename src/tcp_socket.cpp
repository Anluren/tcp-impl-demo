#include "tcp_socket.h"
#include "network_utils.h"
#include <iostream>
#include <algorithm>

namespace tcp_stack {

// Global connection manager (singleton pattern)
static std::shared_ptr<TCPConnectionManager> g_connection_manager = nullptr;
static std::mutex g_manager_mutex;

static std::shared_ptr<TCPConnectionManager> get_connection_manager() {
    std::lock_guard<std::mutex> lock(g_manager_mutex);
    if (!g_connection_manager) {
        g_connection_manager = std::make_shared<TCPConnectionManager>();
        g_connection_manager->initialize();
    }
    return g_connection_manager;
}

TCPSocket::TCPSocket()
    : connection_manager_(get_connection_manager()),
      reliability_(std::make_unique<TCPReliability>()),
      is_listening_(false), is_blocking_(true),
      recv_timeout_(std::chrono::milliseconds(0)),
      send_timeout_(std::chrono::milliseconds(0)),
      local_ip_(0), local_port_(0), should_stop_(false) {}

TCPSocket::~TCPSocket() {
    close();
}

TCPSocket::TCPSocket(TCPSocket&& other) noexcept
    : connection_(std::move(other.connection_)),
      connection_manager_(std::move(other.connection_manager_)),
      reliability_(std::move(other.reliability_)),
      receive_buffer_(std::move(other.receive_buffer_)),
      packet_processor_(std::move(other.packet_processor_)),
      is_listening_(other.is_listening_),
      is_blocking_(other.is_blocking_),
      recv_timeout_(other.recv_timeout_),
      send_timeout_(other.send_timeout_),
      local_ip_(other.local_ip_),
      local_port_(other.local_port_),
      should_stop_(other.should_stop_.load()) {
    other.is_listening_ = false;
    other.should_stop_ = false;
}

TCPSocket& TCPSocket::operator=(TCPSocket&& other) noexcept {
    if (this != &other) {
        close();
        
        connection_ = std::move(other.connection_);
        connection_manager_ = std::move(other.connection_manager_);
        reliability_ = std::move(other.reliability_);
        receive_buffer_ = std::move(other.receive_buffer_);
        packet_processor_ = std::move(other.packet_processor_);
        is_listening_ = other.is_listening_;
        is_blocking_ = other.is_blocking_;
        recv_timeout_ = other.recv_timeout_;
        send_timeout_ = other.send_timeout_;
        local_ip_ = other.local_ip_;
        local_port_ = other.local_port_;
        should_stop_ = other.should_stop_.load();
        
        other.is_listening_ = false;
        other.should_stop_ = false;
    }
    return *this;
}

TCPSocket::TCPSocket(std::shared_ptr<TCPConnection> conn, 
                    std::shared_ptr<TCPConnectionManager> manager)
    : connection_(conn), connection_manager_(manager),
      reliability_(std::make_unique<TCPReliability>()),
      is_listening_(false), is_blocking_(true),
      recv_timeout_(std::chrono::milliseconds(0)),
      send_timeout_(std::chrono::milliseconds(0)),
      local_ip_(conn->local_ip), local_port_(conn->local_port),
      should_stop_(false) {
    
    reliability_->set_initial_seq(conn->local_seq);
    start_packet_processor();
}

bool TCPSocket::bind(const std::string& ip_address, uint16_t port) {
    local_ip_ = resolve_ip_address(ip_address);
    if (local_ip_ == 0) {
        std::cerr << "Failed to resolve IP address: " << ip_address << std::endl;
        return false;
    }
    
    local_port_ = port;
    std::cout << "Socket bound to " << ip_address << ":" << port << std::endl;
    return true;
}

bool TCPSocket::listen(int backlog) {
    if (local_ip_ == 0 || local_port_ == 0) {
        std::cerr << "Socket not bound before listen" << std::endl;
        return false;
    }
    
    if (!connection_manager_->listen(local_ip_, local_port_)) {
        return false;
    }
    
    is_listening_ = true;
    start_packet_processor();
    return true;
}

std::unique_ptr<TCPSocket> TCPSocket::accept() {
    if (!is_listening_) {
        return nullptr;
    }
    
    auto conn = connection_manager_->accept_connection();
    if (!conn) {
        return nullptr;
    }
    
    return std::unique_ptr<TCPSocket>(new TCPSocket(conn, connection_manager_));
}

bool TCPSocket::connect(const std::string& ip_address, uint16_t port) {
    uint32_t remote_ip = resolve_ip_address(ip_address);
    if (remote_ip == 0) {
        std::cerr << "Failed to resolve remote IP: " << ip_address << std::endl;
        return false;
    }
    
    if (local_ip_ == 0) {
        local_ip_ = resolve_ip_address("127.0.0.1"); // Default local IP
    }
    if (local_port_ == 0) {
        local_port_ = 12345; // Default local port - should be randomly assigned
    }
    
    connection_ = connection_manager_->connect(local_ip_, local_port_, remote_ip, port);
    if (!connection_) {
        return false;
    }
    
    reliability_->set_initial_seq(connection_->local_seq);
    start_packet_processor();
    
    // Wait for connection establishment (simplified)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return is_connected();
}

ssize_t TCPSocket::send(const void* data, size_t length) {
    if (!is_connected()) {
        return -1;
    }
    
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> data_vec(bytes, bytes + length);
    
    // Buffer data for reliable transmission
    reliability_->buffer_data(data_vec);
    
    // Send what we can immediately
    size_t total_sent = 0;
    while (total_sent < length && reliability_->can_send_data(1024)) {
        auto chunk = reliability_->get_data_to_send(1024);
        if (chunk.empty()) break;
        
        if (connection_manager_->send_segment(connection_, chunk, TCPHeader::PSH | TCPHeader::ACK)) {
            total_sent += chunk.size();
        } else {
            break;
        }
    }
    
    return total_sent;
}

ssize_t TCPSocket::recv(void* buffer, size_t length) {
    if (!is_connected()) {
        return -1;
    }
    
    std::unique_lock<std::mutex> lock(receive_mutex_);
    
    // Wait for data with timeout if specified
    if (receive_buffer_.empty()) {
        if (recv_timeout_.count() > 0) {
            if (!receive_cv_.wait_for(lock, recv_timeout_, 
                [this] { return !receive_buffer_.empty() || !is_connected(); })) {
                return 0; // Timeout
            }
        } else if (is_blocking_) {
            receive_cv_.wait(lock, [this] { return !receive_buffer_.empty() || !is_connected(); });
        }
    }
    
    if (receive_buffer_.empty()) {
        return 0;
    }
    
    // Copy data to user buffer
    size_t to_copy = std::min(length, receive_buffer_.size());
    std::copy(receive_buffer_.begin(), receive_buffer_.begin() + to_copy,
              static_cast<uint8_t*>(buffer));
    
    receive_buffer_.erase(receive_buffer_.begin(), receive_buffer_.begin() + to_copy);
    
    return to_copy;
}

bool TCPSocket::close() {
    stop_packet_processor();
    
    if (connection_ && connection_->state_machine.is_established()) {
        connection_manager_->close_connection(connection_);
    }
    
    connection_.reset();
    is_listening_ = false;
    return true;
}

bool TCPSocket::is_connected() const {
    return connection_ && connection_->state_machine.is_established();
}

bool TCPSocket::set_blocking(bool blocking) {
    is_blocking_ = blocking;
    return true;
}

bool TCPSocket::set_receive_timeout(std::chrono::milliseconds timeout) {
    recv_timeout_ = timeout;
    return true;
}

bool TCPSocket::set_send_timeout(std::chrono::milliseconds timeout) {
    send_timeout_ = timeout;
    return true;
}

std::string TCPSocket::get_local_address() const {
    return NetworkUtils::ip_network_to_string(local_ip_);
}

uint16_t TCPSocket::get_local_port() const {
    return local_port_;
}

std::string TCPSocket::get_remote_address() const {
    if (connection_) {
        return NetworkUtils::ip_network_to_string(connection_->remote_ip);
    }
    return "";
}

uint16_t TCPSocket::get_remote_port() const {
    return connection_ ? connection_->remote_port : 0;
}

void TCPSocket::packet_processing_loop() {
    while (!should_stop_) {
        // This is a simplified version - in reality, this would integrate
        // more closely with the connection manager's packet processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Process retransmissions
        if (connection_ && reliability_) {
            auto segments_to_retx = reliability_->get_segments_to_retransmit();
            for (auto& segment : segments_to_retx) {
                connection_manager_->send_segment(connection_, segment->data, 
                                                TCPHeader::PSH | TCPHeader::ACK);
                reliability_->mark_segment_sent(segment);
            }
        }
    }
}

void TCPSocket::process_received_data(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(receive_mutex_);
    receive_buffer_.insert(receive_buffer_.end(), data.begin(), data.end());
    receive_cv_.notify_one();
}

uint32_t TCPSocket::resolve_ip_address(const std::string& ip_str) {
    return NetworkUtils::ip_string_to_network(ip_str);
}

bool TCPSocket::start_packet_processor() {
    if (!packet_processor_.joinable()) {
        should_stop_ = false;
        packet_processor_ = std::thread(&TCPSocket::packet_processing_loop, this);
        return true;
    }
    return false;
}

void TCPSocket::stop_packet_processor() {
    should_stop_ = true;
    if (packet_processor_.joinable()) {
        packet_processor_.join();
    }
}

} // namespace tcp_stack