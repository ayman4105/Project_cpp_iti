# Sink Implementations Documentation

## Table of Contents
1. [Overview](#overview)
2. [Sink Interface Pattern](#sink-interface-pattern)
3. [ConsoleSinkImpl Component](#consolesinkimpl-component)
4. [FileSinkImpl Component](#filesinkimpl-component)
5. [Sink Architecture](#sink-architecture)
6. [Performance Considerations](#performance-considerations)
7. [Extension Guide](#extension-guide)
8. [Usage Examples](#usage-examples)

---

## Overview

The **Sink** subsystem provides the output layer of the telemetry logging system. Sinks are responsible for writing formatted log messages to various destinations. This architecture follows the **Strategy Pattern**, allowing multiple output destinations to coexist and be configured independently.

**Available Sink Implementations:**
- `ConsoleSinkImpl`: Writes logs to standard output (terminal)
- `FileSinkImpl`: Writes logs to persistent disk files

**Key Design Principles:**
- **Interface-based**: All sinks implement `ILogSink` interface
- **Polymorphic**: LogManager handles sinks through base interface
- **Composable**: Multiple sinks can operate simultaneously
- **Independent**: Each sink manages its own resources

---

## Sink Interface Pattern

### ILogSink Interface

```cpp
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogMessage &message) = 0;
};
```

**Purpose:**
Defines a contract that all sink implementations must follow.

**Contract:**
- Must implement `write()` method
- Must accept `const LogMessage&` parameter
- Must handle resource cleanup in destructor

**Benefits:**
1. **Decoupling**: LogManager doesn't need to know concrete sink types
2. **Extensibility**: New sinks can be added without modifying existing code
3. **Testability**: Easy to create mock sinks for testing
4. **Flexibility**: Runtime selection and configuration of sinks

### Polymorphic Usage

```cpp
// LogManager stores sinks as base interface pointers
std::vector<std::unique_ptr<ILogSink>> sinks;

// Add different concrete implementations
sinks.push_back(std::make_unique<ConsoleSinkImpl>());
sinks.push_back(std::make_unique<FileSinkImpl>("log.txt"));

// Use polymorphically
for (auto &sink : sinks) {
    sink->write(message);  // Virtual dispatch
}
```

### ASCII Diagram: Sink Hierarchy

```
                    ┌─────────────────┐
                    │   ILogSink      │
                    │  (Interface)    │
                    ├─────────────────┤
                    │ + write(msg)    │
                    └────────┬────────┘
                             │
                             │ implements
                             │
              ┌──────────────┴──────────────┐
              │                             │
              ▼                             ▼
    ┌──────────────────┐         ┌──────────────────┐
    │ ConsoleSinkImpl  │         │  FileSinkImpl    │
    ├──────────────────┤         ├──────────────────┤
    │ + write(msg)     │         │ - file: ofstream │
    │   → std::cout    │         │ + write(msg)     │
    └──────────────────┘         │   → file stream  │
                                 └──────────────────┘
```

---

## ConsoleSinkImpl Component

### Purpose
`ConsoleSinkImpl` provides a simple, immediate-feedback logging destination by writing messages directly to the console (standard output). This is ideal for:
- Development and debugging
- Interactive applications
- Real-time monitoring
- Containerized applications (logs to stdout/stderr)

### Class Structure

```cpp
class ConsoleSinkImpl : public ILogSink {
public:
    void write(const LogMessage &message) override;
};
```

**Characteristics:**
- **Stateless**: No member variables
- **Minimal overhead**: Direct pass-through to `std::cout`
- **Thread-safe (with caveats)**: `std::cout` has internal locking
- **Immediate output**: No buffering beyond `std::cout`'s buffer

### Implementation

```cpp
void ConsoleSinkImpl::write(const LogMessage &message)
{
    std::cout << message;
}
```

**Operation:**
1. Receives `LogMessage` reference
2. Uses overloaded `operator<<` of `LogMessage`
3. Outputs to `std::cout` (standard output stream)

**What Gets Printed:**
The `LogMessage::operator<<` formats the message as:
```
[app_name] [timestamp] [context] [severity_level] [message]
```

**Example Output:**
```
[TelemetryApp] [2025-02-21 14:30:45] [NetworkModule] [WARNING] [Connection timeout]
[TelemetryApp] [2025-02-21 14:30:46] [CPUMonitor] [INFO] [CPU usage: 45%]
```

### Thread Safety

**`std::cout` Locking:**
- C++ standard library provides internal synchronization
- Individual character insertions are atomic
- **However**: Multi-part operations can interleave

**Example of Interleaving:**
```cpp
// Thread 1
std::cout << "[App] [Time] [Context]";  // ← These are separate operations

// Thread 2
std::cout << "[OtherApp] [OtherTime]";

// Possible output (interleaved):
// [App] [OtherApp] [Time] [OtherTime] [Context]
```

**In Practice:**
Since `LogMessage::operator<<` performs a single `<<` operation with a formatted string, interleaving is less likely but not impossible.

**Better Thread Safety (if needed):**
```cpp
class ThreadSafeConsoleSink : public ILogSink {
    std::mutex console_mutex;
    
public:
    void write(const LogMessage &message) override {
        std::lock_guard<std::mutex> lock(console_mutex);
        std::cout << message;
    }
};
```

### Performance Characteristics

| Aspect | Behavior |
|--------|----------|
| **Latency** | Low (direct write) |
| **Throughput** | Limited by terminal speed |
| **Memory** | Minimal (no buffering) |
| **Blocking** | Can block if terminal is slow |
| **Reliability** | Lost if output redirected to closed pipe |

**Console Output Speed:**
- Physical terminal: ~1000 lines/second
- Redirected to file: Much faster
- Piped to another program: Depends on consumer

### Use Cases

✅ **Good For:**
- Development environment debugging
- Docker/Kubernetes (logs to stdout)
- Systemd services (journald captures stdout)
- Quick prototyping
- Real-time monitoring during testing

❌ **Not Ideal For:**
- High-volume production logging (too slow)
- Persistent storage requirements
- Environments without console access
- High-performance scenarios (I/O bound)

### ASCII Diagram: ConsoleSinkImpl Flow

```
┌─────────────────────────────────────────────────────────┐
│              ConsoleSinkImpl::write()                   │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Receives LogMessage
                         │
                         ▼
              ┌──────────────────────┐
              │  std::cout << message│
              └──────────┬───────────┘
                         │
                         │ Uses operator<<
                         │
                         ▼
              ┌──────────────────────┐
              │  LogMessage::        │
              │  operator<<          │
              │  formats message     │
              └──────────┬───────────┘
                         │
                         │ Formatted string
                         │
                         ▼
              ┌──────────────────────┐
              │  std::cout buffer    │
              └──────────┬───────────┘
                         │
                         │ Flush (automatic or manual)
                         │
                         ▼
              ┌──────────────────────┐
              │  Terminal / Console  │
              │  (stdout)            │
              └──────────────────────┘
                         │
                         ▼
           ┌─────────────────────────┐
           │ User sees log in terminal│
           └─────────────────────────┘
```

---

## FileSinkImpl Component

### Purpose
`FileSinkImpl` provides persistent storage of log messages to disk files. This is essential for:
- Production logging
- Audit trails
- Post-mortem debugging
- Long-term analysis
- Compliance requirements

### Class Structure

```cpp
class FileSinkImpl : public ILogSink {
private:
    std::ofstream file;  // Output file stream
    
public:
    FileSinkImpl(const std::string &filename);
    void write(const LogMessage &message) override;
};
```

**State:**
- `std::ofstream file`: Manages the output file stream

**Resource Management:**
- File opened in constructor
- Automatically closed by `std::ofstream` destructor (RAII)
- No explicit close needed

### Constructor

```cpp
FileSinkImpl::FileSinkImpl(const std::string &filename)
    : file(std::ofstream(filename))
{
}
```

**Operation:**
1. Receives filename as parameter
2. Creates `std::ofstream` with filename
3. Initializes member variable using initializer list

**File Opening Behavior:**
```cpp
std::ofstream(filename)
```
- **Mode**: Output mode (write)
- **Behavior**: Creates file if doesn't exist, truncates if exists
- **Error Handling**: Sets `fail()` flag on error, but doesn't throw

**File Permissions:**
Default permissions depend on system `umask`:
- Typically: `0644` (rw-r--r--)
- Owner: read/write
- Group: read
- Others: read

### Improved Constructor (Explicit Modes)

```cpp
FileSinkImpl::FileSinkImpl(const std::string &filename)
    : file(filename, std::ios::out | std::ios::app)
{
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}
```

**Mode Flags:**
- `std::ios::out`: Output mode
- `std::ios::app`: Append mode (don't truncate existing content)
- `std::ios::trunc`: Truncate mode (overwrite)

### Write Method

```cpp
void FileSinkImpl::write(const LogMessage &message)
{
    file << message;
}
```

**Operation:**
1. Receives `LogMessage` reference
2. Uses overloaded `operator<<` to insert into file stream
3. Message formatting handled by `LogMessage::operator<<`

**Buffering:**
`std::ofstream` uses internal buffering:
- Typical buffer size: 4KB - 8KB
- Flushed automatically when:
  - Buffer full
  - Stream destroyed
  - Explicit `flush()` called
  - `std::endl` used (includes flush)

**Write Output Format:**
Same as console output:
```
[app_name] [timestamp] [context] [severity_level] [message]
```

### Error Handling

**Current Implementation:**
- No explicit error checking
- Silently fails if file cannot be opened
- Write errors are silent

**Problems:**
```cpp
FileSinkImpl sink("/read-only-path/log.txt");
// File creation fails, but no exception thrown
sink.write(message);  // Writes to closed stream (no-op)
```

**Improved Error Handling:**
```cpp
void FileSinkImpl::write(const LogMessage &message)
{
    if (!file.is_open()) {
        std::cerr << "[FileSink] File not open, cannot write\n";
        return;
    }
    
    file << message;
    
    if (file.fail()) {
        std::cerr << "[FileSink] Write failed\n";
        file.clear();  // Clear error flags
    }
}
```

### Resource Management (RAII)

**Automatic Cleanup:**
```cpp
{
    FileSinkImpl sink("app.log");
    sink.write(message1);
    sink.write(message2);
    
}  // file.close() automatically called by std::ofstream destructor
```

**Destructor:**
```cpp
~FileSinkImpl() = default;  // Implicit
// std::ofstream destructor:
// 1. Flushes remaining buffer
// 2. Closes file descriptor
// 3. Releases resources
```

### File Flushing Strategies

**Strategy 1: Automatic (Current)**
```cpp
file << message;  // Buffered, flushed eventually
```

**Strategy 2: Force Flush**
```cpp
void write(const LogMessage &message) override {
    file << message << std::flush;  // Immediate disk write
}
```

**Strategy 3: Periodic Flush**
```cpp
class FileSinkImpl {
    int write_count = 0;
    
    void write(const LogMessage &message) override {
        file << message;
        if (++write_count % 10 == 0) {
            file.flush();  // Flush every 10 messages
        }
    }
};
```

**Strategy 4: Line Buffering**
```cpp
void write(const LogMessage &message) override {
    file << message << std::endl;  // Flush after each line
}
```

**Trade-offs:**

| Strategy | Latency | Throughput | Data Loss Risk |
|----------|---------|------------|----------------|
| Automatic | Low | High | Higher (more buffered) |
| Force Flush | High | Low | Minimal (immediate write) |
| Periodic | Medium | Medium | Medium (some buffered) |
| Line Buffering | Medium-High | Medium-Low | Low (per-line flush) |

### Thread Safety

**Problem:**
`std::ofstream` is **NOT** thread-safe. Multiple threads writing simultaneously can cause:
- Interleaved output
- Corrupted log entries
- Undefined behavior

**Example of Data Race:**
```cpp
// Thread 1
sink.write(message1);  // Writes to file buffer

// Thread 2
sink.write(message2);  // Concurrent write to same buffer

// Result: Corrupted file or crash
```

**Solution: Add Mutex**
```cpp
class ThreadSafeFileSink : public ILogSink {
    std::ofstream file;
    std::mutex file_mutex;
    
public:
    void write(const LogMessage &message) override {
        std::lock_guard<std::mutex> lock(file_mutex);
        file << message << std::flush;
    }
};
```

### Performance Characteristics

| Aspect | Behavior |
|--------|----------|
| **Latency** | Medium (buffered writes) |
| **Throughput** | High (buffering improves batch writes) |
| **Memory** | Low (uses std::ofstream buffer) |
| **Reliability** | High (persistent storage) |
| **Disk I/O** | Periodic (based on buffer size) |

**Disk Write Performance:**
- SSD: ~500 MB/s sequential
- HDD: ~100 MB/s sequential
- Network drive: Varies (10-100 MB/s)

### Use Cases

✅ **Good For:**
- Production logging
- Audit trails
- Debugging historical issues
- Compliance and regulatory requirements
- Post-crash analysis
- Long-term storage

❌ **Not Ideal For:**
- Ultra-high-frequency logging (disk becomes bottleneck)
- Real-time systems with strict timing (I/O delays)
- Scenarios requiring guaranteed atomic writes

### ASCII Diagram: FileSinkImpl Flow

```
┌─────────────────────────────────────────────────────────┐
│            FileSinkImpl Lifecycle                       │
└─────────────────────────────────────────────────────────┘

1. CONSTRUCTION
   ┌──────────────────────────────────────┐
   │ FileSinkImpl sink("app.log");        │
   └────────────────┬─────────────────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ std::ofstream file   │
         │ Opens "app.log"      │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ File opened?         │
         │  Yes: Ready to write │
         │  No: fail() set      │
         └──────────────────────┘

2. WRITE OPERATIONS
   ┌──────────────────────────────────────────┐
   │ sink.write(message);                     │
   └────────────────┬─────────────────────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ file << message      │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ std::ofstream buffer │
         │ (4KB - 8KB)          │
         └──────────┬───────────┘
                    │
                    │ Buffer full or flush()
                    │
                    ▼
         ┌──────────────────────┐
         │ Kernel buffer        │
         └──────────┬───────────┘
                    │
                    │ OS write scheduling
                    │
                    ▼
         ┌──────────────────────┐
         │ Physical Disk        │
         │ (SSD/HDD)            │
         └──────────────────────┘

3. DESTRUCTION
   ┌──────────────────────────────────────┐
   │ } // sink goes out of scope          │
   └────────────────┬─────────────────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ ~FileSinkImpl()      │
         │   (implicit)         │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ ~std::ofstream()     │
         │  • Flush buffer      │
         │  • Close file        │
         │  • Release resources │
         └──────────────────────┘
```

### Write Path Detail

```
LogMessage                std::ofstream            OS              Disk
    │                          │                   │               │
    ├─→ write(message) ────────→│                  │               │
    │                          │                   │               │
    │                    ┌─────▼─────┐             │               │
    │                    │User Buffer│             │               │
    │                    │(4-8KB)    │             │               │
    │                    └─────┬─────┘             │               │
    │                          │                   │               │
    │                    [Buffer Full?]            │               │
    │                          │                   │               │
    │                          ├─→ write() ────────→│              │
    │                          │                   │               │
    │                          │            ┌──────▼─────┐         │
    │                          │            │Page Cache  │         │
    │                          │            │(Kernel)    │         │
    │                          │            └──────┬─────┘         │
    │                          │                   │               │
    │                          │            [Dirty Page?]          │
    │                          │                   │               │
    │                          │                   ├─→ fsync() ────→│
    │                          │                   │               │
    │                          │                   │        ┌──────▼─────┐
    │                          │                   │        │Physical    │
    │                          │                   │        │Sectors     │
    │                          │                   │        └────────────┘
```

---

## Sink Architecture

### Multiple Sink Coordination

**LogManager Integration:**
```cpp
class LogManager {
    std::vector<std::unique_ptr<ILogSink>> sinks;
    
    void write() {
        while (auto msg = messages.trypop()) {
            for (auto &sink : sinks) {
                sink->write(msg.value());  // Each sink gets the message
            }
        }
    }
};
```

**Execution Pattern:**
1. LogManager pops message from buffer
2. Iterates through all registered sinks
3. Each sink writes message independently
4. No coordination between sinks

### ASCII Diagram: Multi-Sink Architecture

```
                    ┌────────────────┐
                    │  LogManager    │
                    │                │
                    │ write() called │
                    └────────┬───────┘
                             │
                             │ Pop message from buffer
                             │
                             ▼
                   ┌─────────────────┐
                   │   LogMessage    │
                   │   [msg object]  │
                   └────────┬────────┘
                            │
                            │ Broadcast to all sinks
                            │
         ┌──────────────────┼──────────────────┐
         │                  │                  │
         ▼                  ▼                  ▼
┌─────────────────┐ ┌─────────────────┐ ┌──────────────────┐
│ ConsoleSinkImpl │ │  FileSinkImpl   │ │ FileSinkImpl     │
│                 │ │  ("app.log")    │ │ ("errors.log")   │
├─────────────────┤ ├─────────────────┤ ├──────────────────┤
│ write(msg)      │ │ write(msg)      │ │ write(msg)       │
│   ↓             │ │   ↓             │ │   ↓              │
│ std::cout       │ │ file stream     │ │ file stream      │
└─────────────────┘ └─────────────────┘ └──────────────────┘
         │                  │                  │
         ▼                  ▼                  ▼
    [Terminal]         [app.log]         [errors.log]
```

### Configuration-Driven Setup

**JSON Configuration:**
```json
{
  "sinks": {
    "console": {
      "enabled": true
    },
    "files": [
      {
        "enabled": true,
        "path": "/var/log/telemetry.log"
      },
      {
        "enabled": true,
        "path": "/var/log/telemetry_backup.log"
      }
    ]
  }
}
```

**Runtime Construction:**
```cpp
void TelemetryLoggingApp::setupSinks() {
    // Console sink
    if (config["sinks"]["console"].value("enabled", false)) {
        sinks.push_back(std::make_unique<ConsoleSinkImpl>());
    }
    
    // File sinks
    if (config["sinks"].contains("files")) {
        for (auto &f : config["sinks"]["files"]) {
            if (f.value("enabled", false)) {
                std::string path = f.value("path", "");
                if (!path.empty()) {
                    sinks.push_back(std::make_unique<FileSinkImpl>(path));
                }
            }
        }
    }
}
```

---

## Performance Considerations

### Console vs File Performance

**Throughput Comparison:**

| Sink Type | Messages/Second | Bottleneck |
|-----------|----------------|------------|
| Console (terminal) | ~1,000 | Terminal rendering |
| Console (redirected) | ~100,000 | Pipe buffer |
| File (unbuffered) | ~10,000 | Disk I/O calls |
| File (buffered) | ~100,000 | Buffer capacity |
| File (SSD, buffered) | ~500,000 | CPU serialization |

### Buffering Impact

**Example: 1000 Messages**

**Without Buffering (Force Flush):**
```
Time = 1000 messages × 1ms per disk write = 1000ms
```

**With Buffering (8KB buffer, ~100 messages):**
```
Time = 10 flushes × 1ms per flush = 10ms
```
**Speedup: 100x**

### Optimization Strategies

#### 1. Batch Writes
```cpp
void LogManager::write() {
    std::vector<LogMessage> batch;
    
    // Collect batch
    while (batch.size() < 100 && messages.tryPop(msg)) {
        batch.push_back(msg);
    }
    
    // Write batch to each sink
    for (auto &sink : sinks) {
        for (auto &msg : batch) {
            sink->write(msg);
        }
    }
}
```

#### 2. Asynchronous File Writes
```cpp
class AsyncFileSink : public ILogSink {
    std::ofstream file;
    std::queue<LogMessage> pending;
    std::thread writer_thread;
    
    void writerLoop() {
        while (running) {
            LogMessage msg;
            if (pending.tryPop(msg)) {
                file << msg << std::flush;
            }
        }
    }
};
```

#### 3. Memory-Mapped Files
```cpp
class MmapFileSink : public ILogSink {
    void* mapped_region;
    size_t offset;
    
    void write(const LogMessage &message) override {
        std::string formatted = format(message);
        memcpy(mapped_region + offset, formatted.data(), formatted.size());
        offset += formatted.size();
    }
};
```

---

## Extension Guide

### Creating Custom Sinks

**Step 1: Implement ILogSink Interface**
```cpp
class NetworkSink : public ILogSink {
private:
    SafeSocket socket;
    
public:
    NetworkSink(const std::string &host, uint16_t port)
        : socket(host, port) {}
    
    void write(const LogMessage &message) override {
        std::ostringstream oss;
        oss << message;
        socket.sendString(oss.str());
    }
};
```

**Step 2: Add to Configuration**
```json
{
  "sinks": {
    "network": {
      "enabled": true,
      "host": "log-server.example.com",
      "port": 9000
    }
  }
}
```

**Step 3: Register in Setup**
```cpp
void setupSinks() {
    if (config["sinks"]["network"].value("enabled", false)) {
        std::string host = config["sinks"]["network"]["host"];
        uint16_t port = config["sinks"]["network"]["port"];
        sinks.push_back(std::make_unique<NetworkSink>(host, port));
    }
}
```

### Example Custom Sinks

#### Rotating File Sink
```cpp
class RotatingFileSink : public ILogSink {
    std::string base_path;
    std::ofstream current_file;
    size_t current_size;
    size_t max_size;
    int file_index;
    
    void rotate() {
        current_file.close();
        std::string new_path = base_path + "." + std::to_string(++file_index);
        current_file.open(new_path);
        current_size = 0;
    }
    
public:
    void write(const LogMessage &message) override {
        std::ostringstream oss;
        oss << message;
        std::string formatted = oss.str();
        
        if (current_size + formatted.size() > max_size) {
            rotate();
        }
        
        current_file << formatted;
        current_size += formatted.size();
    }
};
```

#### Syslog Sink
```cpp
class SyslogSink : public ILogSink {
public:
    SyslogSink(const std::string &ident) {
        openlog(ident.c_str(), LOG_PID, LOG_USER);
    }
    
    ~SyslogSink() {
        closelog();
    }
    
    void write(const LogMessage &message) override {
        int priority = LOG_INFO;
        switch (message.level) {
            case severity_level::ERROR:   priority = LOG_ERR; break;
            case severity_level::WARNING: priority = LOG_WARNING; break;
            case severity_level::INFO:    priority = LOG_INFO; break;
            case severity_level::DEBUG:   priority = LOG_DEBUG; break;
        }
        
        syslog(priority, "[%s] %s", 
               message.context.c_str(), 
               message.message.c_str());
    }
};
```

#### Filtered Sink (Severity-Based)
```cpp
class FilteredFileSink : public ILogSink {
    std::unique_ptr<FileSinkImpl> underlying;
    severity_level min_level;
    
public:
    FilteredFileSink(const std::string &path, severity_level min)
        : underlying(std::make_unique<FileSinkImpl>(path)),
          min_level(min) {}
    
    void write(const LogMessage &message) override {
        if (message.level >= min_level) {
            underlying->write(message);
        }
    }
};
```

#### JSON Format Sink
```cpp
class JsonFileSink : public ILogSink {
    std::ofstream file;
    
public:
    JsonFileSink(const std::string &path) : file(path) {}
    
    void write(const LogMessage &message) override {
        nlohmann::json j;
        j["app"] = message.app_name;
        j["timestamp"] = message.time;
        j["context"] = message.context;
        j["level"] = magic_enum::enum_name(message.level);
        j["message"] = message.message;
        
        file << j.dump() << "\n";
    }
};
```

---

## Usage Examples

### Example 1: Single Console Sink
```cpp
#include "LogManager.hpp"
#include "sinks/ConsoleSinkImpl.hpp"

int main() {
    LogManager logger(200, 2);
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
    
    LogMessage msg("MyApp", "Main", "Application started", 
                   severity_level::INFO, "2025-02-21 10:00:00");
    logger.log(msg);
    
    logger.write();  // Flush to console
    return 0;
}
```

**Output:**
```
[MyApp] [2025-02-21 10:00:00] [Main] [INFO] [Application started]
```

### Example 2: Multiple File Sinks
```cpp
LogManager logger(200, 2);

// Main log
logger.add_sink(std::make_unique<FileSinkImpl>("/var/log/app.log"));

// Backup log
logger.add_sink(std::make_unique<FileSinkImpl>("/backup/app.log"));

LogMessage msg("MyApp", "Network", "Connection established", 
               severity_level::INFO, "2025-02-21 10:05:00");
logger.log(msg);
logger.write();  // Written to both files
```

### Example 3: Console + File
```cpp
LogManager logger(200, 2);

// Both outputs
logger.add_sink(std::make_unique<ConsoleSinkImpl>());
logger.add_sink(std::make_unique<FileSinkImpl>("debug.log"));

for (int i = 0; i < 10; i++) {
    LogMessage msg("TestApp", "Loop", 
                   "Iteration " + std::to_string(i), 
                   severity_level::DEBUG, getCurrentTime());
    logger.log(msg);
}

logger.write();  // Appears on console AND in file
```

### Example 4: Conditional Sinks Based on Environment
```cpp
void setupLogging(LogManager &logger, bool is_production) {
    if (is_production) {
        // Production: File only
        logger.add_sink(std::make_unique<FileSinkImpl>("/var/log/prod.log"));
    } else {
        // Development: Console + file
        logger.add_sink(std::make_unique<ConsoleSinkImpl>());
        logger.add_sink(std::make_unique<FileSinkImpl>("dev.log"));
    }
}
```

### Example 5: Error Handling with File Sink
```cpp
try {
    auto sink = std::make_unique<FileSinkImpl>("/invalid/path/log.txt");
    logger.add_sink(std::move(sink));
} catch (const std::exception &e) {
    std::cerr << "Failed to create file sink: " << e.what() << "\n";
    // Fallback to console
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());
}
```

---

## Best Practices

### DO:
✅ Use multiple sinks for redundancy (console + file)
✅ Configure sinks via external config files
✅ Add error checking in custom sink implementations
✅ Consider thread safety for high-concurrency scenarios
✅ Flush files periodically or on critical messages
✅ Use appropriate buffering for performance

### DON'T:
❌ Write to slow sinks in critical paths (use async)
❌ Ignore file open failures
❌ Use console sink in high-performance production
❌ Share file descriptors between sink instances
❌ Forget to flush before application exit

---

## Comparison Summary

| Feature | ConsoleSinkImpl | FileSinkImpl |
|---------|----------------|--------------|
| **Output** | stdout (terminal) | Disk file |
| **Persistence** | No (transient) | Yes (survives restart) |
| **Speed** | Slow (terminal) | Fast (buffered) |
| **Thread Safety** | Partial (std::cout locked) | No (needs mutex) |
| **Resource Usage** | Minimal | Low (file descriptor) |
| **Error Handling** | None | Minimal (should add) |
| **Use Case** | Development, debugging | Production, auditing |
| **State** | Stateless | Stateful (file handle) |
| **Configuration** | None needed | Requires path |

---

## Summary

### ConsoleSinkImpl
- **Simplest** sink implementation
- Direct output to terminal
- No state or configuration
- Ideal for development and debugging
- Limited thread safety

### FileSinkImpl
- **Production-ready** persistent storage
- RAII-based file management
- Configurable output path
- Buffered writes for performance
- Needs improved error handling

### Architecture Benefits
1. **Polymorphic Design**: Easy to add new sink types
2. **Composability**: Multiple sinks work simultaneously
3. **Decoupling**: LogManager independent of sink details
4. **Flexibility**: Runtime configuration of outputs
5. **Extensibility**: Interface-based, open for extension
