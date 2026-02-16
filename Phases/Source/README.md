# Telemetry Logging System Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [LogMessage Component](#logmessage-component)
3. [LogManager Component](#logmanager-component)
4. [TelemetryLoggingApp Component](#telemetryloggingapp-component)
5. [System Architecture](#system-architecture)
6. [Configuration Guide](#configuration-guide)

---

## System Overview

The Telemetry Logging System is a multi-threaded, configurable logging framework designed to collect telemetry data from various sources (files, sockets, SOME/IP), process them according to different policies (CPU, RAM, GPU), and output them to multiple sinks (console, files).

**Key Features:**
- Multi-source telemetry input (File, Socket, SOME/IP)
- Thread-pool based parallel processing
- Lock-free circular buffer for high-performance message queuing
- Multiple output sinks with configurable flush rates
- Policy-based message formatting (CPU, RAM, GPU)
- Graceful signal handling (SIGINT, SIGTERM)

---

## LogMessage Component

### Purpose
`LogMessage` is the fundamental data structure that encapsulates a single log entry in the system. It represents a formatted telemetry message with metadata about the application, context, severity, and timestamp.

### Class Structure

```cpp
class LogMessage {
    std::string app_name;    // Application identifier
    std::string context;     // Context/module name
    std::string message;     // Actual log content
    severity_level level;    // Log severity (INFO, WARNING, ERROR, etc.)
    std::string time;        // Timestamp of the event
};
```

### Constructor

```cpp
LogMessage(const std::string &app, 
           const std::string &cntxt, 
           const std::string &msg, 
           severity_level sev, 
           std::string time)
```

**Parameters:**
- `app`: Name of the application generating the log
- `cntxt`: Context or module within the application
- `msg`: The actual message content
- `sev`: Severity level enumeration
- `time`: Timestamp string

### Output Formatting

The `operator<<` provides a standardized output format:

```
[app_name] [timestamp] [context] [severity] [message]
```

**Example Output:**
```
[TelemetrySystem] [2025-02-16 14:30:45] [CPUMonitor] [WARNING] [CPU usage exceeded 85%]
```

### ASCII Diagram: LogMessage Flow

```
┌─────────────────────────────────────────────────────────────┐
│                      LogMessage Object                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌────────────┐  ┌──────────┐  ┌─────────┐  ┌──────────┐  │
│  │  app_name  │  │ context  │  │ message │  │   time   │  │
│  └────────────┘  └──────────┘  └─────────┘  └──────────┘  │
│        │              │              │            │        │
│        └──────────────┴──────────────┴────────────┘        │
│                          │                                 │
│                          ▼                                 │
│              ┌───────────────────────┐                     │
│              │  operator<< formatter │                     │
│              └───────────────────────┘                     │
│                          │                                 │
│                          ▼                                 │
│         [app][time][context][level][msg]                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Key Implementation Details

1. **Immutable Design**: Once constructed, LogMessage fields are set and ready for consumption
2. **Stream Integration**: Overloaded `operator<<` allows seamless integration with C++ streams
3. **Enum-Based Severity**: Uses `magic_enum` for automatic enum-to-string conversion
4. **Structured Format**: Consistent bracketed format for easy parsing and readability

---

## LogManager Component

### Purpose
`LogManager` is the central coordination component responsible for:
- Managing multiple output sinks
- Queuing incoming log messages in a thread-safe buffer
- Dispatching write operations to a thread pool
- Ensuring messages are delivered to all registered sinks

### Class Structure

```cpp
class LogManager {
private:
    LockFreeCircularBuffer<LogMessage> messages;  // Thread-safe message queue
    std::vector<std::unique_ptr<ILogSink>> sinks; // Output destinations
    std::unique_ptr<ThreadPool> pool;             // Worker threads
    
public:
    LogManager(size_t buffer_capacity, size_t thread_pool_size);
    void add_sink(std::unique_ptr<ILogSink> sink);
    void log(const LogMessage &message);
    void write();
    LogManager& operator<<(const LogMessage &message);
};
```

### Core Methods

#### 1. add_sink()
```cpp
void add_sink(std::unique_ptr<ILogSink> sink)
```
Registers a new output sink (console, file, network, etc.)

**Behavior:**
- Takes ownership of the sink via `std::unique_ptr`
- Adds sink to internal collection
- All future messages will be written to this sink

#### 2. log()
```cpp
void log(const LogMessage &message)
```
Enqueues a message for processing

**Behavior:**
- Attempts to push message into lock-free circular buffer
- If buffer is full, drops the message and logs a warning
- Schedules a write task on the thread pool

**Thread Safety:** Uses lock-free data structure for concurrent access

#### 3. write()
```cpp
void write()
```
Flushes messages from buffer to all sinks

**Behavior:**
- Continuously pops messages from the buffer
- Writes each message to all registered sinks
- Runs until buffer is empty

**Execution Context:** Called by thread pool workers or dedicated writer thread

#### 4. operator<<()
```cpp
LogManager& operator<<(const LogMessage &message)
```
Provides stream-like syntax for logging

**Usage Example:**
```cpp
logManager << LogMessage("App", "Module", "Event occurred", INFO, "12:00:00");
```

### ASCII Diagram: LogManager Architecture

```
                          LogManager
┌───────────────────────────────────────────────────────────────┐
│                                                               │
│   ┌─────────────────────────────────────────────────────┐   │
│   │          log(message) / operator<<(message)         │   │
│   └──────────────────────┬──────────────────────────────┘   │
│                          │                                   │
│                          ▼                                   │
│   ┌──────────────────────────────────────────────────────┐  │
│   │      LockFreeCircularBuffer<LogMessage>             │  │
│   │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐         │  │
│   │  │ M1 │ │ M2 │ │ M3 │ │ M4 │ │ M5 │ │... │         │  │
│   │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘         │  │
│   │         (Thread-safe, lock-free queue)              │  │
│   └──────────────────────┬───────────────────────────────┘  │
│                          │                                   │
│                          ▼                                   │
│   ┌──────────────────────────────────────────────────────┐  │
│   │              ThreadPool (Workers)                    │  │
│   │         ┌────────┐  ┌────────┐  ┌────────┐          │  │
│   │         │Thread 1│  │Thread 2│  │Thread N│          │  │
│   │         └───┬────┘  └───┬────┘  └───┬────┘          │  │
│   │             │           │           │                │  │
│   │             └───────────┴───────────┘                │  │
│   │                      │                               │  │
│   │                      ▼                               │  │
│   │              write() function                        │  │
│   └──────────────────────┬───────────────────────────────┘  │
│                          │                                   │
│                          ▼                                   │
│   ┌──────────────────────────────────────────────────────┐  │
│   │              Sinks Collection                        │  │
│   │  ┌──────────────┐  ┌──────────────┐  ┌───────────┐  │  │
│   │  │ ConsoleSink  │  │  FileSink 1  │  │FileSink 2 │  │  │
│   │  └──────────────┘  └──────────────┘  └───────────┘  │  │
│   └──────────────────────────────────────────────────────┘  │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### Processing Flow

```
1. log(message) called
        ↓
2. tryPush() to circular buffer
        ↓
   ┌────┴────┐
   │         │
Success   Failed (buffer full)
   │         │
   │         └─→ Drop message + warning
   │
   ↓
3. Schedule write() on thread pool
        ↓
4. Worker thread executes write()
        ↓
5. tryPop() messages from buffer
        ↓
6. For each message:
   Write to all sinks
        ↓
7. Repeat until buffer empty
```

### Key Design Decisions

1. **Lock-Free Buffer**: Eliminates contention between producer (log) and consumer (write) threads
2. **Thread Pool**: Prevents unbounded thread creation and manages concurrent write operations
3. **Best-Effort Delivery**: Drops messages on overflow rather than blocking (non-blocking design)
4. **Sink Abstraction**: Interface-based design allows any output destination implementing `ILogSink`

---

## TelemetryLoggingApp Component

### Purpose
`TelemetryLoggingApp` is the top-level orchestration component that:
- Loads configuration from JSON files
- Initializes and manages the LogManager
- Spawns threads for multiple telemetry sources
- Manages periodic sink flushing
- Handles graceful shutdown on system signals

### Class Structure

```cpp
class TelemetryLoggingApp {
private:
    std::unique_ptr<LogManager> logger;
    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::vector<std::thread> sourceThreads;
    std::thread writerThread_;
    nlohmann::json config;
    std::atomic<bool> isRunning;
    
    // Configuration parameters
    size_t buffer_capacity;
    size_t thread_pool_size;
    int sink_flush_rate_ms;
};
```

### Lifecycle Methods

#### 1. Constructor
```cpp
TelemetryLoggingApp(const std::string &configPath)
```

**Operations:**
1. Loads configuration from JSON file
2. Sets up output sinks based on config
3. Creates LogManager with configured parameters
4. Transfers sink ownership to LogManager
5. Registers signal handlers for graceful shutdown

#### 2. Destructor
```cpp
~TelemetryLoggingApp()
```

**Cleanup:**
1. Sets `isRunning` to false
2. Joins all source threads
3. Joins writer thread
4. Allows RAII cleanup of LogManager and sinks

### Configuration Loading

#### loadConfig()
```cpp
void loadConfig(const std::string &path)
```

**Parsed Parameters:**
- `buffer_capacity`: Size of message queue (default: 200)
- `thread_pool_size`: Number of worker threads (default: 2)
- `sink_flush_rate_ms`: Milliseconds between sink flushes (default: 500)

**Error Handling:** Throws `std::runtime_error` if config file cannot be opened

### Sink Setup

#### setupSinks()
```cpp
void setupSinks()
```

**Supported Sinks:**
1. **Console Sink**
   - Enabled via: `config["sinks"]["console"]["enabled"]`
   - Creates: `ConsoleSinkImpl`

2. **File Sinks** (multiple)
   - Enabled via: `config["sinks"]["files"][i]["enabled"]`
   - Path: `config["sinks"]["files"][i]["path"]`
   - Creates: `FileSinkImpl` for each enabled file

### Telemetry Source Setup

#### setupTelemetrySources()
```cpp
void setupTelemetrySources()
```

**Supported Sources:**

1. **File Source**
   - Reads telemetry from a file
   - Configuration:
     - `path`: File location
     - `parse_rate_ms`: Read interval (default: 1000ms)
     - `policy`: Formatting policy (cpu/ram/gpu)

2. **Socket Source**
   - Receives telemetry over TCP socket
   - Configuration:
     - `ip`: Server address (default: 127.0.0.1)
     - `port`: Server port (default: 12345)
     - `parse_rate_ms`: Read interval
     - `policy`: Formatting policy
   - Testing: Use `nc -lk 12345` to simulate server

3. **SOME/IP Source**
   - Automotive middleware integration
   - Configuration:
     - `parse_rate_ms`: Read interval
     - `policy`: Formatting policy
   - Uses singleton pattern: `SomeIPTelemetrySourceImpl::instance()`

**Thread Creation:**
Each enabled source spawns a dedicated thread that:
1. Opens the source connection
2. Reads raw data periodically
3. Formats data according to specified policy
4. Logs formatted message via LogManager
5. Repeats until `isRunning` becomes false

### Policy-Based Formatting

```cpp
std::optional<LogMessage> msg;

if (policy == "cpu")
    msg = Formatter<CPU_policy>::format(raw);
else if (policy == "ram")
    msg = Formatter<RAM_policy>::format(raw);
else if (policy == "gpu")
    msg = Formatter<GPU_policy>::format(raw);
```

Policies define how raw telemetry data is parsed into LogMessage objects.

### Writer Thread

#### startWriterThread()
```cpp
void startWriterThread()
```

**Purpose:** Periodically flushes buffered messages to sinks

**Operation:**
1. Sleeps for `sink_flush_rate_ms` milliseconds
2. Calls `logger->write()` to flush messages
3. Repeats while `isRunning` is true
4. Performs final flush on shutdown

**Design Rationale:** Batching writes reduces I/O overhead and improves throughput

### Signal Handling

#### signalHandler()
```cpp
static void signalHandler(int signal)
```

**Handled Signals:**
- `SIGINT` (Ctrl+C)
- `SIGTERM` (Termination request)

**Behavior:**
1. Sets `isRunning` to false
2. Triggers graceful shutdown
3. Exits application

**Global Instance:** Uses `g_app_instance` to access instance from static context

### Application Start

#### start()
```cpp
void start()
```

**Startup Sequence:**
1. Sets `isRunning` to true
2. Starts writer thread
3. Starts all configured telemetry source threads
4. Blocks main thread until all threads complete (join)

### ASCII Diagram: TelemetryLoggingApp Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                      TelemetryLoggingApp                               │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────┐    │
│  │               Configuration Loading                          │    │
│  │  ┌────────────────────────────────────────────────────┐      │    │
│  │  │  config.json                                       │      │    │
│  │  │  • buffer_capacity                                 │      │    │
│  │  │  • thread_pool_size                                │      │    │
│  │  │  • sink_flush_rate_ms                              │      │    │
│  │  │  • sources: [file, socket, someip]                 │      │    │
│  │  │  • sinks: [console, files]                         │      │    │
│  │  └────────────────────────────────────────────────────┘      │    │
│  └──────────────────────────────────────────────────────────────┘    │
│                              │                                        │
│                              ▼                                        │
│  ┌──────────────────────────────────────────────────────────────┐    │
│  │                    LogManager Instance                       │    │
│  │  • Circular Buffer (capacity configured)                     │    │
│  │  • Thread Pool (size configured)                             │    │
│  │  • Sinks (console + files)                                   │    │
│  └──────────────────────────────────────────────────────────────┘    │
│                              │                                        │
│          ┌───────────────────┼────────────────────┐                  │
│          │                   │                    │                  │
│          ▼                   ▼                    ▼                  │
│  ┌──────────────┐   ┌──────────────┐    ┌──────────────┐            │
│  │ Writer Thread│   │Source Thread │    │Source Thread │            │
│  │              │   │   (File)     │    │  (Socket)    │ ...        │
│  └──────┬───────┘   └──────┬───────┘    └──────┬───────┘            │
│         │                  │                   │                     │
│         │                  │                   │                     │
│  ┌──────▼──────────────────▼───────────────────▼──────────┐         │
│  │            Periodic Execution Loop                      │         │
│  │  ┌───────────────────────────────────────────────────┐  │         │
│  │  │ Writer: sleep(sink_flush_rate_ms)                │  │         │
│  │  │         └─→ logger->write()                      │  │         │
│  │  └───────────────────────────────────────────────────┘  │         │
│  │  ┌───────────────────────────────────────────────────┐  │         │
│  │  │ Sources: sleep(parse_rate_ms)                    │  │         │
│  │  │          └─→ readSource()                        │  │         │
│  │  │              └─→ format(policy)                  │  │         │
│  │  │                  └─→ logger->log(message)        │  │         │
│  │  └───────────────────────────────────────────────────┘  │         │
│  └──────────────────────────────────────────────────────────┘         │
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────┐    │
│  │              Signal Handling (SIGINT/SIGTERM)                │    │
│  │  • Sets isRunning = false                                    │    │
│  │  • Triggers graceful shutdown                                │    │
│  │  • All threads join and cleanup                              │    │
│  └──────────────────────────────────────────────────────────────┘    │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

### Data Flow Through the System

```
Telemetry Source (File/Socket/SOME/IP)
              │
              ▼
    ┌─────────────────┐
    │  readSource()   │
    │  (raw string)   │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ Formatter       │
    │ <CPU/RAM/GPU>   │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │   LogMessage    │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ logger->log()   │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ Circular Buffer │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ Writer Thread   │
    │ (periodic)      │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ logger->write() │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │  Output Sinks   │
    │ (Console/Files) │
    └─────────────────┘
```

### Threading Model

```
Main Thread
    │
    ├─→ Writer Thread (periodic flush)
    │   └─→ Runs every sink_flush_rate_ms
    │       └─→ Calls logger->write()
    │
    ├─→ File Source Thread
    │   └─→ Runs every parse_rate_ms
    │       └─→ Read → Format → Log
    │
    ├─→ Socket Source Thread
    │   └─→ Runs every parse_rate_ms
    │       └─→ Read → Format → Log
    │
    └─→ SOME/IP Source Thread
        └─→ Runs every parse_rate_ms
            └─→ Read → Format → Log

(All threads join on shutdown)
```

---

## System Architecture

### Complete System Overview

```
┌────────────────────────────────────────────────────────────────────┐
│                         APPLICATION LAYER                          │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │              TelemetryLoggingApp (Main)                      │ │
│  │  • Configuration Management                                  │ │
│  │  • Thread Orchestration                                      │ │
│  │  • Signal Handling                                           │ │
│  └──────────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            │                 │                 │
            ▼                 ▼                 ▼
┌─────────────────┐  ┌──────────────┐  ┌──────────────┐
│  INPUT SOURCES  │  │   LOGGING    │  │    OUTPUT    │
│                 │  │   MANAGER    │  │    SINKS     │
├─────────────────┤  ├──────────────┤  ├──────────────┤
│ • FileSrc       │  │ LogManager   │  │ • Console    │
│ • SocketSrc     │─→│              │─→│ • File(s)    │
│ • SOME/IP Src   │  │ (Buffer +    │  │ • Custom     │
│                 │  │  ThreadPool) │  │              │
│ + Formatters    │  │              │  │              │
│   (CPU/RAM/GPU) │  │              │  │              │
└─────────────────┘  └──────────────┘  └──────────────┘
```

### Component Interaction Sequence

```
[App Start]
    │
    1. Load config.json
    │
    2. Create Sinks (Console, Files)
    │
    3. Create LogManager
    │   └─→ Initialize CircularBuffer
    │   └─→ Initialize ThreadPool
    │   └─→ Add all sinks
    │
    4. Start Writer Thread
    │   └─→ Periodic: logger->write()
    │
    5. Start Source Threads
    │   ├─→ File Thread
    │   ├─→ Socket Thread
    │   └─→ SOME/IP Thread
    │
    6. Main thread blocks (join all)
    │
[Runtime: Continuous logging cycle]
    │
    Sources read → Format → Log → Buffer → Flush → Sinks
    │
[Shutdown: SIGINT/SIGTERM]
    │
    7. Set isRunning = false
    │
    8. Join all threads
    │
    9. Final flush
    │
    10. Cleanup (RAII)
    │
[Exit]
```

---

## Configuration Guide

### Configuration File Structure

```json
{
  "log_manager": {
    "buffer_capacity": 200,
    "thread_pool_size": 4,
    "sink_flush_rate_ms": 500
  },
  "sinks": {
    "console": { "enabled": true },
    "files": [
      { "enabled": true, "path": "/home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log" },
      { "enabled": true, "path": "/home/ayman/ITI/Project_cpp_iti/Phases/logs/backup.log" }
    ]
  },
  "sources": {
    "file": {
      "enabled": true,
      "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
      "parse_rate_ms": 1900,
      "policy": "cpu"
    },
    "socket": {
      "enabled": false,
      "ip": "127.0.0.1",
      "port": 12345,
      "parse_rate_ms": 1500,
      "policy": "ram"
    },
    "someip": {
      "enabled": false,
      "parse_rate_ms": 1200,
      "policy": "gpu"
    }
  }
}

```

### Configuration Parameters Explained

#### log_manager Section
- **buffer_capacity**: Maximum number of messages in circular buffer
  - Higher = more buffering, more memory
  - Lower = faster overflow under high load
  - Recommended: 200-1000

- **thread_pool_size**: Number of worker threads for write operations
  - Higher = more parallel processing
  - Lower = less resource usage
  - Recommended: 2-4 (CPU core count dependent)

- **sink_flush_rate_ms**: Milliseconds between sink flushes
  - Higher = better batching, higher latency
  - Lower = lower latency, more I/O
  - Recommended: 100-1000ms

#### sources Section
Each source has:
- **enabled**: Boolean to activate/deactivate
- **parse_rate_ms**: Read interval in milliseconds
- **policy**: Formatting policy ("cpu", "ram", "gpu")
- Source-specific fields (path, ip, port, etc.)

#### sinks Section
- **console**: Simple enable/disable
- **files**: Array of file sinks with path and enable flag

---

## Performance Considerations

### Lock-Free Design
- Circular buffer uses atomic operations
- No mutex contention between producers and consumers
- Scales well with multiple source threads

### Thread Pool Benefits
- Prevents thread explosion under high load
- Reuses threads for multiple write operations
- Bounded resource consumption

### Buffering Strategy
- Messages batched before disk writes
- Reduces I/O system calls
- Trade-off: Potential message loss on crash before flush

### Overflow Handling
- Non-blocking: Drops messages when buffer full
- Prevents cascade failures
- Logs dropped message warnings

---

## Thread Safety Guarantees

1. **LogManager::log()**: Thread-safe via lock-free buffer
2. **Source Threads**: Independent, no shared state
3. **Writer Thread**: Single consumer pattern
4. **Signal Handler**: Atomic flag (`std::atomic<bool>`)

---

## Extensibility Points

### Adding New Sources
1. Implement telemetry source interface
2. Add configuration schema
3. Add thread creation in `setupTelemetrySources()`

### Adding New Sinks
1. Implement `ILogSink` interface
2. Add configuration parsing in `setupSinks()`
3. Register with LogManager

### Adding New Policies
1. Define policy structure
2. Implement `Formatter<YourPolicy>::format()`
3. Add conditional branch in source threads

---

## Error Handling

### Configuration Errors
- Missing config file: `std::runtime_error` thrown
- Invalid JSON: Exception propagated to caller

### Runtime Errors
- Buffer overflow: Message dropped, warning logged
- Source connection failure: Thread continues retrying
- Sink write failure: Dependent on sink implementation

### Shutdown Handling
- Graceful: All threads joined
- Final buffer flush performed
- RAII ensures resource cleanup

---

## Best Practices

1. **Buffer Sizing**: Set `buffer_capacity` > peak message rate × `sink_flush_rate_ms`
2. **Thread Pool**: Match CPU cores for compute-bound tasks
3. **Flush Rate**: Balance latency requirements with I/O efficiency
4. **Source Rate**: Avoid overwhelming buffer with too-frequent reads
5. **Testing**: Use `nc -lk <port>` for socket testing
6. **Monitoring**: Watch for "buffer full" warnings

---

## Summary

The Telemetry Logging System provides a robust, high-performance framework for collecting, processing, and outputting telemetry data. Its key strengths are:

- **Modularity**: Sources, sinks, and policies are independently configurable
- **Performance**: Lock-free buffers and thread pools maximize throughput
- **Reliability**: Graceful degradation and signal handling ensure stability
- **Extensibility**: Interface-based design allows easy addition of components

This architecture is suitable for embedded systems, automotive applications, and any scenario requiring high-throughput, low-latency logging of telemetry data.

