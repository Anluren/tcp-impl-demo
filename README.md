# TCP Stack Implementation

A comprehensive C++17 implementation of a TCP/IP stack from scratch, featuring all core TCP protocols and a complete networking stack.

## ✅ Complete Implementation

This project implements **all TCP protocols** including:

- **Raw socket interface** for low-level packet transmission
- **IP layer** with packet creation, parsing, and checksums  
- **Complete TCP state machine** (RFC 793 compliant)
- **TCP connection management** (3-way handshake, teardown)
- **Reliability features** (sequence numbers, ACKs, retransmission, flow control)
- **High-level socket API** similar to BSD sockets
- **RTT estimation** and adaptive retransmission timeouts
- **Multi-threaded design** with background packet processing

## Building

```bash
mkdir build
cd build
cmake -G Ninja ..  # Using Ninja for fast builds
ninja
```

## Running Tests

```bash
# Run the test suite
./tests/tcp_tests
```

## Running Examples

**Note: Raw socket operations require root privileges**

```bash
# Terminal 1 - Start the server
sudo ./examples/tcp_server

# Terminal 2 - Start the client  
sudo ./examples/tcp_client
```

## Requirements

- Linux (for raw socket support)
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.12+
- Ninja build system (recommended)
- **Root privileges** (for raw socket operations)

## Project Structure

```
├── src/                    # Source files
│   ├── raw_socket.cpp     # Low-level socket operations
│   ├── ip_layer.cpp       # IP packet handling
│   ├── tcp_state_machine.cpp # TCP state transitions
│   ├── tcp_connection_manager.cpp # Connection handling
│   ├── tcp_reliability.cpp # Reliability features
│   ├── tcp_socket.cpp     # High-level socket API
│   └── network_utils.cpp  # Utility functions
├── include/               # Header files
├── examples/              # Sample applications
│   ├── server.cpp        # TCP echo server
│   └── client.cpp        # Interactive TCP client
├── tests/                 # Test suite
└── build/                 # Build directory
```

## Usage

### Server Example
```cpp
#include "tcp_socket.h"

TCPSocket server;
server.bind("127.0.0.1", 8080);
server.listen();

auto client = server.accept();
char buffer[1024];
ssize_t bytes = client->recv(buffer, sizeof(buffer));
client->send("Hello", 5);
```

### Client Example
```cpp
#include "tcp_socket.h"

TCPSocket client;
client.connect("127.0.0.1", 8080);
client.send("Hello Server", 12);

char response[1024];
ssize_t bytes = client.recv(response, sizeof(response));
```

See examples/ directory for complete client and server implementations.

## Architecture

```
┌─────────────────┐
│   Application   │  ← Examples: server.cpp, client.cpp
├─────────────────┤
│   TCP Socket    │  ← High-level BSD-like API
├─────────────────┤
│ TCP Connection  │  ← Connection management & handshake
│   Management    │
├─────────────────┤
│ TCP Reliability │  ← Sequence numbers, ACKs, retransmission
├─────────────────┤
│ TCP State       │  ← RFC 793 state machine
│   Machine       │
├─────────────────┤
│   IP Layer      │  ← Packet creation, parsing, checksums
├─────────────────┤
│  Raw Sockets    │  ← Low-level packet transmission
└─────────────────┘
```

## Key Features

### 🔌 **Raw Socket Interface**
- Cross-platform raw socket wrapper
- Non-blocking I/O support
- Comprehensive error handling

### 🌐 **IP Layer**
- RFC 791 compliant IPv4 implementation
- Automatic checksum calculation
- Packet validation and parsing

### 🔄 **TCP State Machine** 
- All 11 TCP states (CLOSED, LISTEN, SYN_SENT, etc.)
- RFC 793 compliant state transitions
- Event-driven architecture

### 🤝 **Connection Management**
- Complete 3-way handshake
- Graceful connection teardown
- Multiple connection support

### 🛡️ **Reliability Features**
- Sequence number management
- Acknowledgment handling
- Adaptive retransmission (RFC 6298)
- RTT estimation and RTO calculation
- Flow control and window management

### 🔗 **Socket API**
- BSD sockets-like interface
- Blocking and non-blocking modes
- Timeout support
- Thread-safe operations

## Technical Details

- **Modern C++17** with smart pointers, move semantics, and RAII
- **Multi-threaded design** with background packet processing
- **Memory-safe** implementation
- **Ninja build system** for fast incremental builds
- **Comprehensive test suite** with unit and integration tests

## Implementation Status

✅ **Complete TCP Stack Implementation**
- All core protocols implemented
- Full RFC compliance for TCP state machine  
- Production-quality error handling
- Comprehensive test coverage
- Example applications included

## Educational Value

This implementation demonstrates:
- **Network protocol design** and implementation
- **System-level programming** with raw sockets
- **Multi-threaded network programming**
- **Modern C++17** best practices
- **State machine design patterns**
- **Real-time systems** with timing constraints

## Warning

⚠️ **Important Notes:**
- This is an **educational implementation** - not intended for production use
- **Root privileges required** for raw socket operations  
- Raw socket operations can **interfere with system networking**
- Designed for **Linux systems** (can be adapted for other platforms)
- May require **firewall configuration** to allow raw socket access

## Contributing

This project serves as a comprehensive example of TCP/IP implementation. 
Contributions for educational improvements, additional protocols, or 
cross-platform compatibility are welcome.

## License

Educational use. See individual file headers for specific licensing terms.