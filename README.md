# Transport-CPP

Transport-CPP is a modern C++17 networking library that provides a unified, object-oriented interface for all common networking paradigms. It abstracts complex networking operations into clean class hierarchies where ownership is clearly managed and connections are tied to object lifetimes. The library supports both synchronous and asynchronous operations, with the Engine providing high-performance event-driven I/O using the OS's native polling mechanisms.

## Features

- **Unified Interface**: All networking paradigms (TCP, UDP, Serial) follow consistent patterns
- **Object Lifetime Management**: Connections are automatically managed through RAII
- **Sync/Async Support**: Choose between synchronous operations or high-performance async I/O
- **High Performance**: Uses OS-native polling (epoll/kqueue) for efficient event handling
- **Type Safety**: Strong typing with comprehensive error handling
- **Cross-Platform**: Supports Linux, Windows, and macOS
- **Timer Support**: High-precision timers with callback functionality for periodic tasks
- **Modern C++**: Leverages C++17 features for clean, expressive code

## Architecture Overview

Transport-CPP is built around several key concepts:

### Device Hierarchy
- **Device**: Base class for all I/O operations
- **IODevice**: Extends Device with generic I/O capabilities (sync/async send/receive)
- **NetworkDevice**: Extends IODevice with network-specific features
- **Timer**: Extends Device with high-precision timing and callback functionality
- **Specialized Devices**: TCP clients/servers, UDP clients/servers, Serial ports

### Engine
The Engine class provides asynchronous I/O capabilities:
- Registers devices for polling
- Uses OS-native polling mechanisms (`poll()` on Linux)
- Handles multiple concurrent connections efficiently
- Provides event callbacks for ready-to-read/write states

### Synchronous vs Asynchronous
- **Synchronous**: Direct send/receive operations that block until completion
- **Asynchronous**: Non-blocking operations managed by the Engine with callbacks

## Quick Start

### Basic TCP Client (Synchronous)

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/networking/tcpclient.h>

using namespace Context::Devices::IO::Networking;
using namespace Context::Devices::IO::Networking::TCP;

int main() {
    Client tcpClient;
    
    // Connect to server
    HostAddr server = {"127.0.0.1", 8080};
    if (tcpClient.connectToHost(server) == RETURN::OK) {
        // Send data synchronously
        IODevice::IODATA message = {'H', 'e', 'l', 'l', 'o'};
        auto response = tcpClient.syncRequestResponse(message);
        
        if (response.code == RETURN::OK && response.result) {
            // Process response data
            const auto& data = response.result.value();
            std::cout << "Received: ";
            for (auto byte : data) {
                std::cout << static_cast<char>(byte);
            }
            std::cout << std::endl;
        }
    }
    
    return 0;
}
```

### Basic TCP Server (Asynchronous)

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/networking/tcpserver.h>
#include <transport-cpp/engine.h>

using namespace Context::Devices::IO::Networking;
using namespace Context::Devices::IO::Networking::TCP;

int main() {
    Context::Engine engine;
    Server::Acceptor server;
    
    // Set up new peer handler
    server.setNewPeerHandler([](std::unique_ptr<Server::Peer> peer) {
        std::cout << "New client connected from: " 
                  << peer->getPeerAddr().ip << ":" << peer->getPeerAddr().port << std::endl;
        
        // Set request handler for this peer
        peer->setRequestHandler([](const NetworkMessage& request) -> std::optional<IODevice::IODATA> {
            // Echo the received data back
            return request.data;
        });
        
        // Set disconnect handler
        peer->setDisconnectHandler([](Server::Peer* peer) {
            std::cout << "Client disconnected" << std::endl;
        });
    });
    
    // Bind server to port
    if (server.bind(8080) == RETURN::OK) {
        // Register server with engine for async operations
        engine.registerDevice(server);
        
        std::cout << "Server listening on port 8080..." << std::endl;
        
        // Run event loop
        engine.awaitForever();
    }
    
    return 0;
}
```

### UDP Communication

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/networking/udpclient.h>
#include <transport-cpp/networking/udpserver.h>

using namespace Context::Devices::IO::Networking;
using namespace Context::Devices::IO::Networking::UDP;

// UDP Client
void udpClientExample() {
    Client udpClient;
    HostAddr server = {"127.0.0.1", 8080};
    
    if (udpClient.connectToHost(server) == RETURN::OK) {
        IODevice::IODATA message = {'U', 'D', 'P', ' ', 'H', 'i'};
        auto response = udpClient.syncRequestResponse(message);
        
        if (response.code == RETURN::OK && response.result) {
            // Handle response
        }
    }
}

// UDP Server
void udpServerExample() {
    Context::Engine engine;
    Server udpServer;
    
    // Set up message handler
    udpServer.setGenericNetworkCallback([&](const NetworkMessage& message) {
        std::cout << "Received UDP message from " 
                  << message.peer.ip << ":" << message.peer.port << std::endl;
        
        // Echo back
        IODevice::IODATA response = {'E', 'c', 'h', 'o'};
        udpServer.sendTo(message.peer, response);
    });
    
    if (udpServer.bind(8080) == RETURN::OK) {
        engine.registerDevice(udpServer);
        engine.awaitForever();
    }
}
```

### Serial Communication

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/io/serial.h>

using namespace Context::Devices::IO;

int main() {
    Serial serialPort;
    
    // Configure serial settings
    Serial::Settings settings;
    settings.baud = 115200;
    settings.bitsPerByte = Serial::Bits::B8;
    settings.enableParity = false;
    settings.use2StopBits = false;
    
    Serial::Device device = {"/dev/ttyUSB0", settings};
    
    if (serialPort.openDevice(device) == RETURN::OK) {
        // Set up async callback
        serialPort.setIODataCallback([](const IODevice::IODATA& data) {
            std::cout << "Received serial data: ";
            for (auto byte : data) {
                std::cout << static_cast<char>(byte);
            }
            std::cout << std::endl;
        });
        
        // Send data
        IODevice::IODATA message = {'A', 'T', '\r', '\n'};
        serialPort.asyncSend(message);
        
        // For async operations, register with engine
        Context::Engine engine;
        engine.registerDevice(serialPort);
        engine.awaitFor(std::chrono::seconds(10));
    }
    
    return 0;
}
```

### Timer Usage

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/timer.h>
#include <transport-cpp/engine.h>

using namespace Context;

int main() {
    Engine engine;
    Timer periodicTimer;
    
    // Set up timer callback
    periodicTimer.setCallback([]() {
        std::cout << "Timer triggered at: " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now().time_since_epoch()).count()
                  << "ms" << std::endl;
    });
    
    // Start timer with 1-second interval
    if (periodicTimer.start(std::chrono::milliseconds(1000)) == RETURN::OK) {
        // Register timer with engine for async operation
        engine.registerDevice(periodicTimer);
        
        std::cout << "Timer started, will trigger every second..." << std::endl;
        
        // Run for 10 seconds
        engine.awaitFor(std::chrono::seconds(10));
        
        // Stop the timer
        periodicTimer.stop();
    }
    
    return 0;
}
```

### Combining Timers with Network Operations

```cpp
#include <transport-cpp/transport-cpp.h>
#include <transport-cpp/timer.h>
#include <transport-cpp/networking/tcpclient.h>
#include <transport-cpp/engine.h>

using namespace Context;
using namespace Context::Devices::IO::Networking::TCP;

int main() {
    Engine engine;
    Client tcpClient;
    Timer heartbeatTimer;
    
    // Connect to server
    HostAddr server = {"127.0.0.1", 8080};
    if (tcpClient.connectToHost(server) == RETURN::OK) {
        
        // Set up data callback for incoming messages
        tcpClient.setIODataCallback([](const IODevice::IODATA& data) {
            std::cout << "Received: ";
            for (auto byte : data) {
                std::cout << static_cast<char>(byte);
            }
            std::cout << std::endl;
        });
        
        // Set up heartbeat timer
        heartbeatTimer.setCallback([&tcpClient]() {
            IODevice::IODATA heartbeat = {'H', 'E', 'A', 'R', 'T', 'B', 'E', 'A', 'T'};
            tcpClient.asyncSend(heartbeat);
            std::cout << "Heartbeat sent" << std::endl;
        });
        
        // Register both devices with engine
        engine.registerDevice(tcpClient);
        engine.registerDevice(heartbeatTimer);
        
        // Start heartbeat every 30 seconds
        heartbeatTimer.start(std::chrono::seconds(30));
        
        // Run event loop
        engine.awaitFor(std::chrono::minutes(5));
        
        // Clean up
        heartbeatTimer.stop();
        tcpClient.disconnect();
    }
    
    return 0;
}
```

### One-shot Timer

```cpp
// Timer for delayed execution
Timer delayTimer;

delayTimer.setCallback([]() {
    std::cout << "Delayed task executed!" << std::endl;
    // Perform delayed task here
});

// Start timer for single execution (need to stop after first trigger)
if (delayTimer.start(std::chrono::milliseconds(5000)) == RETURN::OK) {
    engine.registerDevice(delayTimer);
    
    // Timer will trigger once after 5 seconds
    // For one-shot behavior, stop the timer in the callback:
    delayTimer.setCallback([&delayTimer]() {
        std::cout << "One-shot timer executed!" << std::endl;
        delayTimer.stop();
    });
}
```

## Advanced Usage

### Custom Logging

```cpp
#include <transport-cpp/transport-cpp.h>

// Create custom logger
auto logger = std::make_shared<Transport::Logger>();
logger->setMinimumLogLevel(Transport::Logger::LogLevel::INFO);

// Apply to engine
Context::Engine engine;
engine.setLogger(logger);

// Apply to devices
TCP::Client client;
client.setLogger(logger);
```

### Error Handling

```cpp
TCP::Client client;
HostAddr server = {"invalid.host", 8080};

if (client.connectToHost(server) != RETURN::OK) {
    auto error = client.getLastError();
    
    if (std::holds_alternative<Context::Device::ERROR_CODE>(error.code)) {
        auto deviceError = std::get<Context::Device::ERROR_CODE>(error.code);
        std::cout << "Device Error: " << Context::Device::errCodeToString(deviceError)
                  << " - " << error.description << std::endl;
    } else {
        auto sysError = std::get<SYS_ERR_CODE>(error.code);
        std::cout << "System Error: " << sysError 
                  << " - " << error.description << std::endl;
    }
}
```

### Engine Management

```cpp
Context::Engine engine;

// Register multiple devices
TCP::Client client1, client2;
UDP::Server udpServer;

engine.registerDevice(client1);
engine.registerDevice(client2);
engine.registerDevice(udpServer);

// Different waiting strategies
engine.awaitOnce();                                    // Poll once, return immediately
engine.awaitOnce(std::chrono::milliseconds(100));     // Poll with timeout
engine.awaitFor(std::chrono::seconds(5));             // Poll for specific duration
engine.awaitForever();                                 // Poll indefinitely

// Clean deregistration
engine.deRegisterDevice(client1);
```

## Building and Installation

### Requirements
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.15+
- Linux/Unix system (for current implementation)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd transport-cpp

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

### Using in Your Project

#### CMake Integration

```cmake
find_package(transport-cpp REQUIRED)
target_link_libraries(your_target transport::transport-cpp)
```

#### Manual Integration

```cmake
add_subdirectory(path/to/transport-cpp)
target_link_libraries(your_target transport-cpp)
```

## API Reference

### Core Classes

- **`Context::Engine`**: Event loop manager for asynchronous operations
- **`Context::Device`**: Base class for all I/O devices
- **`Context::Timer`**: High-precision timer with callback functionality
- **`Context::Devices::IO::IODevice`**: Generic I/O device with send/receive capabilities
- **`Context::Devices::IO::Networking::NetworkDevice`**: Network-specific device base

### TCP Classes
- **`TCP::Client`**: TCP client for outgoing connections
- **`TCP::Server::Acceptor`**: TCP server that accepts incoming connections
- **`TCP::Server::Peer`**: Represents a connected client on the server side

### UDP Classes
- **`UDP::Client`**: UDP client for point-to-point communication
- **`UDP::Server`**: UDP server for handling incoming datagrams
- **`UDP::Broadcaster`**: UDP broadcaster for subnet broadcasting
- **`UDP::Multicaster`**: UDP multicaster for multicast communication

### Serial Classes
- **`Serial`**: Serial port communication with configurable settings

### Timer Classes
- **`Timer`**: High-precision timer using Linux `timerfd` with callback support
  - **`start(duration)`**: Start/restart timer with specified interval
  - **`stop()`**: Stop the timer
  - **`resume()`**: Resume timer with previously set duration
  - **`setCallback(func)`**: Set callback function to execute on timer trigger
  - **`isRunning()`**: Check if timer is currently active

### Data Types
- **`IODevice::IODATA`**: `std::vector<int8_t>` - primary data container
- **`HostAddr`**: Structure containing IP address and port
- **`NetworkMessage`**: Container for network data with peer information

## Performance Considerations

- Use asynchronous operations with the Engine for handling multiple concurrent connections
- The Engine uses OS-native polling (poll/epoll) for efficient I/O multiplexing
- Synchronous operations are suitable for simple request-response patterns
- Device registration/deregistration with Engine is thread-safe
- Timers use Linux `timerfd` for high-precision timing with ~1000ns accuracy
- Timer callbacks are executed in the Engine's event loop thread

## Thread Safety

- Individual device operations are generally not thread-safe
- Engine operations are designed to be called from a single thread
- Use separate Engine instances for multi-threaded applications
- Callback functions should be thread-aware when accessing shared data

## License

This project is licensed under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please ensure code follows the existing style and includes appropriate tests.

## Version

Current version: 0.1

---

Transport-CPP provides a clean, modern interface for network programming in C++. Its object-oriented design and clear ownership model make it easy to build robust networked applications while maintaining high performance through asynchronous I/O capabilities.