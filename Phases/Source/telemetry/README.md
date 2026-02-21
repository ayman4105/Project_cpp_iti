# Telemetry Source Implementations Documentation

## Table of Contents
1. [Overview](#overview)
2. [Source Interface Pattern](#source-interface-pattern)
3. [FileTelemetrySrc Component](#filetelemetrysrc-component)
4. [SocketTelemetrySrc Component](#sockettelemetrysrc-component)
5. [SomeIPTelemetrySourceImpl Component](#someiptelemetrysourceimpl-component)
6. [Source Architecture](#source-architecture)
7. [Integration Patterns](#integration-patterns)
8. [Main Function Examples](#main-function-examples)

---

## Overview

The **Telemetry Source** subsystem provides the input layer of the telemetry logging system. Sources are responsible for acquiring raw telemetry data from various origins and delivering it to the logging pipeline for formatting and output.

**Available Source Implementations:**
- `FileTelemetrySrc`: Reads telemetry data from files
- `SocketTelemetrySrc`: Receives telemetry data over TCP sockets
- `SomeIPTelemetrySourceImpl`: Integrates with SOME/IP automotive middleware

**Key Design Principles:**
- **Abstraction**: Common interface for different data sources
- **Lazy Opening**: Sources opened on-demand via `openSource()`
- **Line-Based Protocol**: Sources read data line-by-line
- **Resource Management**: Uses RAII wrappers (SafeFile, SafeSocket)
- **Optional Pattern**: Sources use `std::optional` for validity checking

---

## Source Interface Pattern

### ITelemetrySource Interface (Implied)

```cpp
class ITelemetrySource {
public:
    virtual ~ITelemetrySource() = default;
    virtual bool openSource() = 0;
    virtual bool readSource(std::string &out) = 0;
};
```

**Contract:**
- `openSource()`: Initialize connection/open resource
- `readSource()`: Read next data item (line) into output string
- Return `false` on error, `true` on success

**Benefits:**
1. **Uniform Interface**: All sources accessed the same way
2. **Hot-Swappable**: Sources can be changed at runtime
3. **Testability**: Easy to mock for testing
4. **Composability**: Multiple sources can coexist

### ASCII Diagram: Source Hierarchy

```
                    ┌─────────────────────┐
                    │ ITelemetrySource    │
                    │   (Interface)       │
                    ├─────────────────────┤
                    │ + openSource()      │
                    │ + readSource(out)   │
                    └──────────┬──────────┘
                               │
                               │ implements
                               │
         ┌─────────────────────┼─────────────────────┐
         │                     │                     │
         ▼                     ▼                     ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│FileTelemetrySrc  │  │SocketTelemetry   │  │SomeIPTelemetry   │
│                  │  │Src               │  │SourceImpl        │
├──────────────────┤  ├──────────────────┤  ├──────────────────┤
│- path: string    │  │- ip: string      │  │- proxy: shared   │
│- file: optional  │  │- port: uint16_t  │  │- runtime: shared │
│  <SafeFile>      │  │- sock: optional  │  │- last_usage:float│
│                  │  │  <SafeSocket>    │  │- has_new_data    │
├──────────────────┤  ├──────────────────┤  ├──────────────────┤
│+ openSource()    │  │+ openSource()    │  │+ openSource()    │
│+ readSource(out) │  │+ readSource(out) │  │+ start()         │
└──────────────────┘  └──────────────────┘  │+ readSource(out) │
                                            └──────────────────┘
```

---

## FileTelemetrySrc Component

### Purpose
`FileTelemetrySrc` reads telemetry data from files, treating each line as a separate telemetry event. This is useful for:
- Replaying recorded telemetry sessions
- Testing with static datasets
- Reading from log files or sensor dumps
- Simulating real-time data from pre-recorded sources

### Class Structure

```cpp
class FileTelemetrySrc {
private:
    std::string path;                    // File path
    std::optional<SafeFile> file;        // File wrapper (lazy-initialized)
    
public:
    FileTelemetrySrc(std::string path);
    bool openSource();
    bool readSource(std::string &out);
};
```

**Key Design Decisions:**
- **`std::optional<SafeFile>`**: File not opened until `openSource()` called
- **Move Semantics**: Path moved into member to avoid copying
- **Validity Checking**: `if (!file)` checks if file is opened

### Constructor

```cpp
FileTelemetrySrc::FileTelemetrySrc(string path)
    : path(std::move(path))
{
}
```

**Operation:**
1. Accepts file path as parameter
2. Moves path into member variable (efficient, avoids copy)
3. Does **NOT** open file yet (deferred initialization)

**Why Defer Opening?**
- Constructor might be called before file is ready
- Allows error handling at appropriate time
- Separates construction from initialization

### Open Source

```cpp
bool FileTelemetrySrc::openSource()
{
    file = SafeFile(path);
    return true;
}
```

**Operation:**
1. Constructs `SafeFile` with stored path
2. Assigns to `std::optional` (makes it "engaged")
3. Always returns `true` (optimistic)

**Important Note:**
Current implementation doesn't check if file open succeeded. Better implementation:

```cpp
bool FileTelemetrySrc::openSource()
{
    file = SafeFile(path);
    // SafeFile should have is_open() method
    return file.has_value() && file->is_open();
}
```

### Read Source

```cpp
bool FileTelemetrySrc::readSource(string &out)
{
    if (!file) {
        return false;
    }
    return file.value().readLine(out);
}
```

**Operation:**
1. **Validity Check**: `if (!file)` ensures file is opened
2. **Unwrap Optional**: `file.value()` gets SafeFile reference
3. **Delegate**: Calls `SafeFile::readLine()` which:
   - Reads characters until newline
   - Returns `true` if line read successfully
   - Returns `false` on EOF or error

**Return Values:**
- `true`: Line successfully read into `out`
- `false`: File not opened OR EOF reached OR read error

**Data Flow:**
```
FileTelemetrySrc::readSource()
         │
         ├─→ Check: file opened?
         │   No → return false
         │   Yes ↓
         │
         └─→ file.value().readLine(out)
                    │
                    └─→ SafeFile reads byte-by-byte
                            │
                            └─→ out = "telemetry line"
```

### ASCII Diagram: FileTelemetrySrc Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│             FileTelemetrySrc Lifecycle                      │
└─────────────────────────────────────────────────────────────┘

1. CONSTRUCTION
   ┌──────────────────────────────────────────────┐
   │ FileTelemetrySrc src("/data/sensors.log");   │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ path = "...log"      │
              │ file = std::nullopt  │
              │ (not opened yet)     │
              └──────────────────────┘

2. OPEN SOURCE
   ┌──────────────────────────────────────────────┐
   │ src.openSource();                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ file = SafeFile(path)│
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ file.has_value()     │
              │ = true               │
              │ (ready to read)      │
              └──────────────────────┘

3. READ LOOP
   ┌──────────────────────────────────────────────┐
   │ while (src.readSource(data)) {               │
   │     // process data                          │
   │ }                                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ Check: file opened?       │
         │ Yes → readLine(out)       │
         └───────────┬───────────────┘
                     │
         ┌───────────▼───────────┐
         │ SafeFile::readLine()  │
         │ Read until '\n'       │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │ out = "sensor_data"   │
         │ return true           │
         └───────────────────────┘

4. END OF FILE
   ┌──────────────────────────────────────────────┐
   │ src.readSource(data) returns false           │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ EOF reached          │
              │ Loop exits           │
              └──────────────────────┘

5. DESTRUCTION
   ┌──────────────────────────────────────────────┐
   │ } // src goes out of scope                   │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ ~FileTelemetrySrc()  │
              │   (implicit)         │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ file.reset()         │
              │ → ~SafeFile()        │
              │   → close(fd)        │
              └──────────────────────┘
```

### Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                  File Source Data Flow                      │
└─────────────────────────────────────────────────────────────┘

[Physical File]
      │
      │ sensors.log:
      │   line1: CPU 45.2
      │   line2: RAM 78.5
      │   line3: GPU 92.1
      │
      ▼
┌──────────────┐
│  SafeFile    │
│  (fd)        │
└──────┬───────┘
       │
       │ readLine()
       │
       ▼
┌──────────────┐
│ out = string │
│ "CPU 45.2"   │
└──────┬───────┘
       │
       │ return true
       │
       ▼
┌──────────────────┐
│ FileTelemetrySrc │
│ readSource()     │
└──────┬───────────┘
       │
       │ return to caller
       │
       ▼
┌──────────────────┐
│ Source Thread    │
│ in LoggingApp    │
└──────────────────┘
```

---

## SocketTelemetrySrc Component

### Purpose
`SocketTelemetrySrc` receives telemetry data over TCP network connections. This enables:
- Remote telemetry collection
- Distributed system monitoring
- Real-time data streaming from network devices
- Integration with external telemetry sources

### Class Structure

```cpp
class SocketTelemetrySrc {
private:
    std::string ip;                      // Server IP address
    uint16_t port;                       // Server port
    std::optional<SafeSocket> sock;      // Socket wrapper (lazy-initialized)
    
public:
    SocketTelemetrySrc(std::string _ip, uint16_t _port);
    bool openSource();
    bool readSource(std::string &out);
};
```

**Key Characteristics:**
- **Network-Based**: Connects to remote telemetry server
- **Lazy Connection**: Socket not opened until needed
- **Line Protocol**: Expects newline-delimited messages
- **Reconnection-Friendly**: Can recreate socket if connection fails

### Constructor

```cpp
SocketTelemetrySrc::SocketTelemetrySrc(string _ip, uint16_t _port)
    : ip(std::move(_ip)), port(_port)
{
}
```

**Operation:**
1. Stores IP address (moved for efficiency)
2. Stores port number (value type, copied)
3. Does **NOT** establish connection yet

**Design Rationale:**
- Network might not be ready at construction time
- Connection errors better handled in `openSource()`
- Allows configuration before connection

### Open Source

```cpp
bool SocketTelemetrySrc::openSource()
{
    sock = SafeSocket(ip, port);
    return true;
}
```

**Operation:**
1. Constructs `SafeSocket` with IP and port
2. SafeSocket constructor:
   - Creates socket
   - Connects to server
   - Throws exception on failure
3. Assigns to `std::optional`

**Exception Handling:**
Current implementation doesn't catch exceptions. If connection fails:
- Exception propagates to caller
- Source remains unopened
- Better to wrap in try-catch

**Improved Version:**
```cpp
bool SocketTelemetrySrc::openSource()
{
    try {
        sock = SafeSocket(ip, port);
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Socket open failed: " << e.what() << "\n";
        return false;
    }
}
```

### Read Source

```cpp
bool SocketTelemetrySrc::readSource(string &out)
{
    if (!sock) {
        return false;
    }
    return sock.value().receiveLine(out);
}
```

**Operation:**
1. **Validity Check**: Ensures socket is connected
2. **Unwrap Optional**: Gets SafeSocket reference
3. **Delegate**: Calls `SafeSocket::receiveLine()` which:
   - Reads bytes until newline character
   - Returns `true` if line received
   - Returns `false` on connection close or error

**Network Protocol:**
```
Server sends:  "CPU 67.3\n"
Client reads:  out = "CPU 67.3" (without \n)
```

**Return Values:**
- `true`: Message successfully received
- `false`: Socket not opened OR connection closed OR network error

### ASCII Diagram: SocketTelemetrySrc Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│           SocketTelemetrySrc Lifecycle                      │
└─────────────────────────────────────────────────────────────┘

1. CONSTRUCTION
   ┌──────────────────────────────────────────────┐
   │ SocketTelemetrySrc src("192.168.1.100",     │
   │                        9000);                │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ ip = "192.168.1.100" │
              │ port = 9000          │
              │ sock = std::nullopt  │
              │ (not connected)      │
              └──────────────────────┘

2. OPEN SOURCE
   ┌──────────────────────────────────────────────┐
   │ src.openSource();                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ sock = SafeSocket    │
              │        (ip, port)    │
              └──────────┬───────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ TCP Connection Sequence:  │
         │ 1. socket()               │
         │ 2. connect()              │
         │ 3. 3-way handshake        │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ sock.has_value() = true   │
         │ (connected, ready)        │
         └───────────────────────────┘

3. READ LOOP
   ┌──────────────────────────────────────────────┐
   │ while (src.readSource(data)) {               │
   │     // process network data                  │
   │ }                                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ Check: socket connected?  │
         │ Yes → receiveLine(out)    │
         └───────────┬───────────────┘
                     │
         ┌───────────▼───────────┐
         │ SafeSocket::          │
         │ receiveLine()         │
         │ recv() until '\n'     │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │ out = "network_data"  │
         │ return true           │
         └───────────────────────┘

4. CONNECTION CLOSED
   ┌──────────────────────────────────────────────┐
   │ src.readSource(data) returns false           │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ Server closed or     │
              │ network error        │
              │ Loop exits           │
              └──────────────────────┘

5. DESTRUCTION
   ┌──────────────────────────────────────────────┐
   │ } // src goes out of scope                   │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ ~SocketTelemetrySrc()│
              │   (implicit)         │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ sock.reset()         │
              │ → ~SafeSocket()      │
              │   → close(sockfd)    │
              └──────────────────────┘
```

### Network Communication Diagram

```
┌─────────────────────────────────────────────────────────────┐
│              Socket Source Network Flow                     │
└─────────────────────────────────────────────────────────────┘

Client (SocketTelemetrySrc)          Server (Telemetry Source)
         │                                     │
         ├──→ openSource()                     │
         │    socket()                         │
         │    connect() ─────────────────────→ │
         │                                     │
         │ ←──── ACK (TCP handshake) ─────────┤
         │                                     │
    [Connected]                           [Connected]
         │                                     │
         │                                     │
         ├──→ readSource()                     │
         │    recv() waiting... ←──────────────┤ send("CPU 45\n")
         │                                     │
         │    out = "CPU 45"                   │
         │    return true                      │
         │                                     │
         ├──→ readSource()                     │
         │    recv() waiting... ←──────────────┤ send("RAM 78\n")
         │                                     │
         │    out = "RAM 78"                   │
         │    return true                      │
         │                                     │
         │                                     │
         │                          ┌──────────┤ Server closes
         │                          │          │
         ├──→ readSource()          │          │
         │    recv() ───────────────┴────────→ X (connection closed)
         │    return false                     │
         │                                     │
    [Disconnected]                       [Disconnected]
```

### Testing with Netcat

**Server Simulation:**
```bash
# Terminal 1: Start telemetry server
nc -lk 12345

# Type telemetry data:
CPU 45.2
RAM 67.8
GPU 92.1
```

**Client Connection:**
```bash
# Terminal 2: Your application connects
# SocketTelemetrySrc("127.0.0.1", 12345)
# Receives lines as you type them in Terminal 1
```

---

## SomeIPTelemetrySourceImpl Component

### Purpose
`SomeIPTelemetrySourceImpl` integrates with **SOME/IP** (Scalable service-Oriented MiddlewarE over IP), an automotive middleware standard. This enables:
- Integration with automotive ECUs (Electronic Control Units)
- Service-oriented communication in vehicles
- Event-driven telemetry updates
- Request-response data retrieval

### Class Structure

```cpp
class SomeIPTelemetrySourceImpl {
private:
    std::shared_ptr<CommonAPI::Runtime> runtime;
    std::shared_ptr<v1::omnimetron::gpu::GpuUsageDataProxy> proxy;
    float last_usage;
    std::atomic<bool> has_new_data;
    std::mutex data_mutex;
    
    // Singleton pattern
    SomeIPTelemetrySourceImpl();
    
public:
    static SomeIPTelemetrySourceImpl& instance();
    bool openSource();
    bool start();
    bool readSource(std::string &out);
};
```

**Key Characteristics:**
- **Singleton Pattern**: Only one instance exists system-wide
- **Event-Driven**: Subscribes to GPU usage change events
- **Request-Response**: Can query current GPU usage on-demand
- **Thread-Safe**: Uses mutex and atomics for data access
- **SOME/IP Proxy**: Uses CommonAPI framework

### Singleton Pattern

```cpp
SomeIPTelemetrySourceImpl& SomeIPTelemetrySourceImpl::instance()
{
    static SomeIPTelemetrySourceImpl inst;
    return inst;
}
```

**Operation:**
1. Uses C++11 static local variable initialization
2. Thread-safe (guaranteed by C++ standard)
3. Lazy initialization (created on first call)
4. Same instance returned on subsequent calls

**Why Singleton?**
- SOME/IP middleware expects single connection per service
- Global coordination of event subscriptions
- Resource sharing across application

**Usage Pattern:**
```cpp
auto &source = SomeIPTelemetrySourceImpl::instance();
source.openSource();
```

### Constructor

```cpp
SomeIPTelemetrySourceImpl::SomeIPTelemetrySourceImpl()
    : last_usage(0.0f), has_new_data(false)
{
}
```

**Operation:**
1. Initializes `last_usage` to 0.0
2. Initializes `has_new_data` flag to false
3. Does **NOT** create runtime or proxy (deferred)

**Private Constructor:**
Prevents direct instantiation, enforcing singleton pattern.

### Open Source

```cpp
bool SomeIPTelemetrySourceImpl::openSource()
{
    runtime = CommonAPI::Runtime::get();
    proxy = runtime->buildProxy<v1::omnimetron::gpu::GpuUsageDataProxy>(
        DOMAIN, INSTANCE);
    return proxy != nullptr;
}
```

**Operation:**
1. **Get Runtime**: Obtains CommonAPI runtime singleton
2. **Build Proxy**: Creates proxy for GPU usage service
   - Domain: "local" (service location)
   - Instance: "omnimetron.gpu.GpuUsageData" (service identifier)
3. **Validation**: Returns true if proxy successfully created

**SOME/IP Proxy:**
- Acts as client-side stub for remote service
- Provides methods and events defined in service interface
- Handles serialization, communication, and deserialization

### Start Method

```cpp
bool SomeIPTelemetrySourceImpl::start()
{
    if (!proxy)
        return false;
    
    auto &event = proxy->getNotifyGpuUsageDataChangeEvent();
    event.subscribe([this](const float &usage) {
        std::lock_guard<std::mutex> lock(data_mutex);
        last_usage = usage;
        has_new_data.store(true);
    });
    
    return true;
}
```

**Operation:**
1. **Validation**: Checks if proxy exists
2. **Get Event**: Obtains reference to GPU usage change event
3. **Subscribe**: Registers callback for event notifications
4. **Callback Lambda**:
   - Locks mutex for thread safety
   - Updates `last_usage` with new value
   - Sets `has_new_data` flag

**Event-Driven Model:**
```
GPU Service (ECU)              SomeIPTelemetrySourceImpl
       │                                  │
       │  Usage changes: 45% → 67%        │
       │                                  │
       ├─→ Notify event ─────────────────→│
       │                                  │
       │                         Callback executes:
       │                         lock(data_mutex)
       │                         last_usage = 67.0
       │                         has_new_data = true
       │                                  │
```

### Read Source

```cpp
bool SomeIPTelemetrySourceImpl::readSource(std::string &out)
{
    if (!proxy)
        return false;
    
    CommonAPI::CallStatus status;
    float usage = 0.0f;
    
    proxy->requestGpuUsageData(status, usage, nullptr);
    
    if (status != CommonAPI::CallStatus::SUCCESS)
        return false;
    
    out = std::to_string(usage);
    return true;
}
```

**Operation:**
1. **Validation**: Checks proxy existence
2. **Synchronous Call**: Requests current GPU usage
   - `status`: Output parameter for call result
   - `usage`: Output parameter for data
   - `nullptr`: No callback (synchronous)
3. **Status Check**: Validates call succeeded
4. **Conversion**: Converts float to string
5. **Return**: Indicates success or failure

**Request-Response Model:**
```
SomeIPTelemetrySourceImpl          GPU Service (ECU)
         │                                  │
         ├─→ requestGpuUsageData() ────────→│
         │                                  │
         │                          Processing...
         │                          Read sensor
         │                                  │
         │ ←─── Response: 67.3 ─────────────┤
         │                                  │
    out = "67.3"
    return true
```

### ASCII Diagram: SomeIPTelemetrySourceImpl Architecture

```
┌─────────────────────────────────────────────────────────────┐
│        SomeIPTelemetrySourceImpl Architecture               │
└─────────────────────────────────────────────────────────────┘

                  ┌───────────────────┐
                  │   Application     │
                  │   (Source Thread) │
                  └─────────┬─────────┘
                            │
                            │ instance()
                            │
                            ▼
              ┌─────────────────────────┐
              │ SomeIPTelemetry         │
              │ SourceImpl (Singleton)  │
              ├─────────────────────────┤
              │ - runtime               │
              │ - proxy                 │
              │ - last_usage            │
              │ - has_new_data          │
              │ - data_mutex            │
              └─────────┬───────────────┘
                        │
                        │ buildProxy()
                        │
                        ▼
              ┌─────────────────────────┐
              │ CommonAPI Runtime       │
              └─────────┬───────────────┘
                        │
                        │ creates
                        │
                        ▼
              ┌─────────────────────────┐
              │ GpuUsageDataProxy       │
              ├─────────────────────────┤
              │ + requestGpuUsageData() │
              │ + NotifyEvent           │
              └─────────┬───────────────┘
                        │
                        │ SOME/IP Protocol
                        │
                        ▼
              ┌─────────────────────────┐
              │ GPU Service (ECU)       │
              │ Provides GPU usage data │
              └─────────────────────────┘
```

### Lifecycle Diagram

```
┌─────────────────────────────────────────────────────────────┐
│      SomeIPTelemetrySourceImpl Lifecycle                    │
└─────────────────────────────────────────────────────────────┘

1. SINGLETON ACCESS
   ┌──────────────────────────────────────────────┐
   │ auto &src = SomeIPTelemetrySourceImpl::     │
   │             instance();                      │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ First call creates   │
              │ static instance      │
              │ (thread-safe)        │
              └──────────────────────┘

2. OPEN SOURCE
   ┌──────────────────────────────────────────────┐
   │ src.openSource();                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ runtime = Runtime::get()  │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ proxy = buildProxy<...>(  │
         │   "local",                │
         │   "omnimetron.gpu.Gpu...")│
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ Proxy created and ready   │
         └───────────────────────────┘

3. START (EVENT SUBSCRIPTION)
   ┌──────────────────────────────────────────────┐
   │ src.start();                                 │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ event = proxy->get        │
         │   NotifyGpuUsageData      │
         │   ChangeEvent()           │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ event.subscribe(lambda)   │
         │ Lambda captures [this]    │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ Listening for events      │
         │ (async, callback-based)   │
         └───────────────────────────┘

4. RUNTIME OPERATION (TWO MODES)

   MODE A: EVENT-DRIVEN UPDATE
   ┌──────────────────────────────────────────────┐
   │ GPU usage changes on ECU                     │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ Event notification sent   │
         │ via SOME/IP               │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ Lambda callback executes: │
         │  lock(data_mutex)         │
         │  last_usage = new_value   │
         │  has_new_data = true      │
         └───────────────────────────┘

   MODE B: REQUEST-RESPONSE
   ┌──────────────────────────────────────────────┐
   │ src.readSource(out);                         │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────┐
         │ proxy->requestGpuUsage    │
         │ Data(status, usage, null) │
         └───────────┬───────────────┘
                     │
                     │ Synchronous call
                     │ Blocks until response
                     │
                     ▼
         ┌───────────────────────────┐
         │ Response received         │
         │ usage = 67.3              │
         │ status = SUCCESS          │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ out = "67.3"              │
         │ return true               │
         └───────────────────────────┘

5. DESTRUCTION
   ┌──────────────────────────────────────────────┐
   │ Application exits                            │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ Static instance      │
              │ destroyed            │
              │ (automatic cleanup)  │
              └──────────────────────┘
```

### Thread Safety Analysis

```
┌─────────────────────────────────────────────────────────────┐
│                Thread Safety Mechanisms                     │
└─────────────────────────────────────────────────────────────┘

PROTECTED DATA:
  - last_usage (float)
  - has_new_data (atomic<bool>)

CONCURRENT ACCESS POINTS:

Thread 1: Event Callback        Thread 2: readSource()
         │                              │
         │                              │
         ├─→ Event arrives               │
         │   Lambda executes:            │
         │   lock(data_mutex) ─────┐     │
         │   last_usage = 67.3     │     │
         │   has_new_data = true   │     │
         │   unlock                │     │
         │                         │     │
         │                         └─────┼─→ Blocks here if
         │                               │   mutex locked
         │                               │
         │                               ├─→ requestGpu()
         │                               │   (separate call)
         │                               │
         │                               │   No race with
         │                               │   last_usage
         │                               │
         │                               ├─→ return result
         │                               │
```

---

## Source Architecture

### Comparison Matrix

| Feature | FileTelemetrySrc | SocketTelemetrySrc | SomeIPTelemetrySourceImpl |
|---------|------------------|--------------------|-----------------------------|
| **Data Origin** | Local file | Network socket | SOME/IP service |
| **Connection Type** | File I/O | TCP | Middleware RPC |
| **Initialization** | Open file | Connect socket | Build proxy |
| **Read Model** | Synchronous | Synchronous | Synchronous/Event |
| **Thread Safety** | N/A (single reader) | N/A (single reader) | Mutex + atomics |
| **Reconnection** | Reopen file | Reconnect socket | Rebuild proxy |
| **Latency** | Low (local) | Medium (network) | Low-Medium (IPC) |
| **Use Case** | Testing, replay | Remote monitoring | Automotive ECU |
| **Pattern** | Pull | Pull | Pull + Push |

### Unified Access Pattern

```cpp
// All sources accessed uniformly
std::unique_ptr<ITelemetrySource> source;

if (use_file) {
    source = std::make_unique<FileTelemetrySrc>("/data/log");
} else if (use_socket) {
    source = std::make_unique<SocketTelemetrySrc>("server", 9000);
} else {
    source = &SomeIPTelemetrySourceImpl::instance();
}

source->openSource();

std::string data;
while (source->readSource(data)) {
    // Process data uniformly
    processData(data);
}
```

---

## Integration Patterns

### Integration with TelemetryLoggingApp

**Source Thread Creation:**
```cpp
void TelemetryLoggingApp::setupTelemetrySources()
{
    // FILE source
    if (config["sources"]["file"].value("enabled", false)) {
        std::string path = config["sources"]["file"]["path"];
        int rate = config["sources"]["file"]["parse_rate_ms"];
        
        sourceThreads.emplace_back([this, path, rate]() {
            auto source = std::make_unique<FileTelemetrySrc>(path);
            
            if (!source->openSource())
                return;
            
            while (isRunning) {
                std::string raw;
                if (source->readSource(raw)) {
                    auto msg = Formatter<CPU_policy>::format(raw);
                    if (msg.has_value())
                        logger->log(msg.value());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(rate));
            }
        });
    }
    
    // SOCKET source
    if (config["sources"]["socket"].value("enabled", false)) {
        std::string ip = config["sources"]["socket"]["ip"];
        uint16_t port = config["sources"]["socket"]["port"];
        int rate = config["sources"]["socket"]["parse_rate_ms"];
        
        sourceThreads.emplace_back([this, ip, port, rate]() {
            auto source = std::make_unique<SocketTelemetrySrc>(ip, port);
            
            while (isRunning) {
                if (source->openSource()) {
                    std::string raw;
                    if (source->readSource(raw)) {
                        auto msg = Formatter<RAM_policy>::format(raw);
                        if (msg.has_value())
                            logger->log(msg.value());
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(rate));
            }
        });
    }
    
    // SOMEIP source
    if (config["sources"]["someip"].value("enabled", false)) {
        int rate = config["sources"]["someip"]["parse_rate_ms"];
        
        sourceThreads.emplace_back([this, rate]() {
            auto &source = SomeIPTelemetrySourceImpl::instance();
            
            if (!source.openSource())
                return;
            
            source.start();  // Subscribe to events
            
            while (isRunning) {
                std::string raw;
                if (source.readSource(raw)) {
                    auto msg = Formatter<GPU_policy>::format(raw);
                    if (msg.has_value())
                        logger->log(msg.value());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(rate));
            }
        });
    }
}
```

### Data Flow Through System

```
┌─────────────────────────────────────────────────────────────┐
│           Complete Telemetry Source Flow                    │
└─────────────────────────────────────────────────────────────┘

[Data Origins]
     │
     ├─→ File: /var/log/sensors.log
     │        ↓
     │   FileTelemetrySrc
     │        ↓
     │   readSource() → "CPU 45.2"
     │
     ├─→ Network: 192.168.1.100:9000
     │        ↓
     │   SocketTelemetrySrc
     │        ↓
     │   readSource() → "RAM 78.5"
     │
     └─→ SOME/IP: GPU Service
              ↓
         SomeIPTelemetrySourceImpl
              ↓
         readSource() → "92.1"
              │
              └────────┬────────────────────┘
                       │
                       ▼
              ┌────────────────┐
              │ Source Thread  │
              │ (per source)   │
              └────────┬───────┘
                       │
                       │ Format with policy
                       │
                       ▼
              ┌────────────────┐
              │ Formatter      │
              │ <CPU_policy>   │
              │ <RAM_policy>   │
              │ <GPU_policy>   │
              └────────┬───────┘
                       │
                       │ Creates LogMessage
                       │
                       ▼
              ┌────────────────┐
              │  LogMessage    │
              └────────┬───────┘
                       │
                       │ logger->log()
                       │
                       ▼
              ┌────────────────┐
              │  LogManager    │
              │  (buffer)      │
              └────────┬───────┘
                       │
                       ▼
              ┌────────────────┐
              │  Sinks         │
              │ (console/file) │
              └────────────────┘
```

---

## Main Function Examples

### Example 1: File Source Only

```cpp
#include "telemetry/FileTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "sinks/ConsoleSinkImpl.hpp"

int main() {
    // Create log manager
    LogManager logger(200, 2);
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
    
    // Create file source
    FileTelemetrySrc source("/var/log/sensors.txt");
    
    if (!source.openSource()) {
        std::cerr << "Failed to open file\n";
        return 1;
    }
    
    // Read and log all data
    std::string raw_data;
    while (source.readSource(raw_data)) {
        LogMessage msg("TelemetryApp", "FileSrc", 
                       raw_data, severity_level::INFO, 
                       getCurrentTime());
        logger.log(msg);
    }
    
    // Flush to sinks
    logger.write();
    
    return 0;
}
```

**Expected Behavior:**
1. Opens `/var/log/sensors.txt`
2. Reads each line sequentially
3. Creates log message for each line
4. Outputs to console
5. Exits when file ends

### Example 2: Socket Source with Reconnection

```cpp
#include "telemetry/SocketTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "sinks/FileSinkImpl.hpp"
#include <thread>
#include <chrono>

int main() {
    LogManager logger(200, 2);
    logger.add_sink(std::make_unique<FileSinkImpl>("network_telemetry.log"));
    
    SocketTelemetrySrc source("192.168.1.100", 9000);
    
    while (true) {
        try {
            if (!source.openSource()) {
                std::cerr << "Connection failed, retrying in 5s...\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            
            std::cout << "Connected to telemetry server\n";
            
            std::string data;
            while (source.readSource(data)) {
                LogMessage msg("NetworkApp", "SocketSrc", 
                               data, severity_level::INFO, 
                               getCurrentTime());
                logger.log(msg);
                logger.write();
            }
            
            std::cout << "Connection closed, reconnecting...\n";
            
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    return 0;
}
```

**Expected Behavior:**
1. Attempts to connect to server
2. If fails, waits 5 seconds and retries
3. Once connected, reads data continuously
4. Logs each message to file
5. On disconnection, automatically reconnects
6. Runs indefinitely

### Example 3: SOME/IP Source with Events

```cpp
#include "telemetry/SomeIPTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "sinks/ConsoleSinkImpl.hpp"
#include <thread>
#include <chrono>

int main() {
    LogManager logger(200, 2);
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
    
    auto &source = SomeIPTelemetrySourceImpl::instance();
    
    if (!source.openSource()) {
        std::cerr << "Failed to connect to SOME/IP service\n";
        return 1;
    }
    
    if (!source.start()) {
        std::cerr << "Failed to start event subscription\n";
        return 1;
    }
    
    std::cout << "Monitoring GPU usage (Ctrl+C to exit)...\n";
    
    // Poll for data every second
    for (int i = 0; i < 60; i++) {
        std::string usage_data;
        if (source.readSource(usage_data)) {
            LogMessage msg("GPUMonitor", "SomeIPSrc", 
                           "GPU Usage: " + usage_data + "%", 
                           severity_level::INFO, 
                           getCurrentTime());
            logger.log(msg);
            logger.write();
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

**Expected Behavior:**
1. Gets singleton instance
2. Opens SOME/IP proxy connection
3. Subscribes to GPU usage events
4. Polls for data every second (60 times)
5. Logs GPU usage to console
6. Exits after 60 seconds

### Example 4: Multi-Source Application

```cpp
#include "telemetry/FileTelemetrySourceImpl.hpp"
#include "telemetry/SocketTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "sinks/ConsoleSinkImpl.hpp"
#include "sinks/FileSinkImpl.hpp"
#include <thread>
#include <atomic>

std::atomic<bool> running{true};

void fileSourceThread(LogManager &logger) {
    FileTelemetrySrc source("/var/log/cpu_data.txt");
    
    if (!source.openSource()) {
        std::cerr << "File source failed\n";
        return;
    }
    
    std::string data;
    while (running && source.readSource(data)) {
        LogMessage msg("MultiApp", "FileSrc", 
                       "CPU: " + data, severity_level::INFO, 
                       getCurrentTime());
        logger.log(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void socketSourceThread(LogManager &logger) {
    SocketTelemetrySrc source("localhost", 8888);
    
    while (running) {
        try {
            if (!source.openSource()) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
            
            std::string data;
            while (running && source.readSource(data)) {
                LogMessage msg("MultiApp", "SocketSrc", 
                               "Network: " + data, severity_level::INFO, 
                               getCurrentTime());
                logger.log(msg);
            }
        } catch (...) {
            std::cerr << "Socket error, retrying...\n";
        }
    }
}

void writerThread(LogManager &logger) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        logger.write();
    }
    logger.write();  // Final flush
}

int main() {
    LogManager logger(500, 4);
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
    logger.add_sink(std::make_unique<FileSinkImpl>("multi_source.log"));
    
    std::thread file_thread(fileSourceThread, std::ref(logger));
    std::thread socket_thread(socketSourceThread, std::ref(logger));
    std::thread writer_thread(writerThread, std::ref(logger));
    
    std::cout << "Press Enter to stop...\n";
    std::cin.get();
    
    running = false;
    
    file_thread.join();
    socket_thread.join();
    writer_thread.join();
    
    std::cout << "All sources stopped\n";
    return 0;
}
```

**Expected Behavior:**
1. Starts three threads:
   - File source thread (reads CPU data)
   - Socket source thread (reads network data)
   - Writer thread (flushes to sinks every 500ms)
2. Both sources feed into same LogManager
3. Output goes to console AND file
4. Runs until user presses Enter
5. Gracefully shuts down all threads

### Example 5: Policy-Based Formatting

```cpp
#include "telemetry/FileTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "Formatter.hpp"
#include "sinks/ConsoleSinkImpl.hpp"

int main() {
    LogManager logger(200, 2);
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
    
    FileTelemetrySrc cpu_source("cpu_readings.txt");
    FileTelemetrySrc ram_source("ram_readings.txt");
    FileTelemetrySrc gpu_source("gpu_readings.txt");
    
    cpu_source.openSource();
    ram_source.openSource();
    gpu_source.openSource();
    
    std::string raw;
    
    // Read and format CPU data
    while (cpu_source.readSource(raw)) {
        auto msg = Formatter<CPU_policy>::format(raw);
        if (msg.has_value()) {
            logger.log(msg.value());
        }
    }
    
    // Read and format RAM data
    while (ram_source.readSource(raw)) {
        auto msg = Formatter<RAM_policy>::format(raw);
        if (msg.has_value()) {
            logger.log(msg.value());
        }
    }
    
    // Read and format GPU data
    while (gpu_source.readSource(raw)) {
        auto msg = Formatter<GPU_policy>::format(raw);
        if (msg.has_value()) {
            logger.log(msg.value());
        }
    }
    
    logger.write();
    
    return 0;
}
```

**Expected Behavior:**
1. Opens three separate file sources
2. Applies different formatting policies:
   - CPU_policy for CPU data
   - RAM_policy for RAM data
   - GPU_policy for GPU data
3. All formatted messages logged to same manager
4. Output to console with appropriate formatting

### Example 6: Error Handling and Validation

```cpp
#include "telemetry/SocketTelemetrySourceImpl.hpp"
#include "LogManager.hpp"
#include "sinks/FileSinkImpl.hpp"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }
    
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);
    
    LogManager logger(200, 2);
    
    try {
        logger.add_sink(std::make_unique<FileSinkImpl>("telemetry.log"));
    } catch (const std::exception &e) {
        std::cerr << "Failed to create log file: " << e.what() << "\n";
        return 1;
    }
    
    SocketTelemetrySrc source(ip, port);
    
    int retry_count = 0;
    const int max_retries = 5;
    
    while (retry_count < max_retries) {
        try {
            if (!source.openSource()) {
                throw std::runtime_error("Connection failed");
            }
            
            std::cout << "Connected successfully\n";
            break;
            
        } catch (const std::exception &e) {
            std::cerr << "Attempt " << (retry_count + 1) 
                      << " failed: " << e.what() << "\n";
            retry_count++;
            
            if (retry_count >= max_retries) {
                std::cerr << "Max retries reached, exiting\n";
                return 1;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    std::string data;
    int message_count = 0;
    
    while (source.readSource(data)) {
        LogMessage msg("TelemetryClient", "Network", 
                       data, severity_level::INFO, 
                       getCurrentTime());
        logger.log(msg);
        
        message_count++;
        if (message_count % 10 == 0) {
            logger.write();
            std::cout << "Processed " << message_count << " messages\n";
        }
    }
    
    logger.write();
    std::cout << "Total messages: " << message_count << "\n";
    
    return 0;
}
```

**Expected Behavior:**
1. Validates command-line arguments
2. Creates log file with error handling
3. Attempts connection with retry logic (max 5 attempts)
4. Reads and processes messages
5. Flushes every 10 messages for performance
6. Reports progress and final count

---

## Summary

### FileTelemetrySrc
- Reads telemetry from local files
- Line-by-line processing
- Uses SafeFile RAII wrapper
- Suitable for testing and replay scenarios

### SocketTelemetrySrc
- Receives telemetry over TCP network
- Uses SafeSocket RAII wrapper
- Supports reconnection patterns
- Ideal for distributed monitoring

### SomeIPTelemetrySourceImpl
- Integrates with automotive middleware
- Singleton pattern for global coordination
- Event-driven and request-response models
- Thread-safe data access
- Production automotive use cases
