# Complete System Integration Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Main Application Entry Point](#main-application-entry-point)
3. [Configuration File Structure](#configuration-file-structure)
4. [CMake Build System](#cmake-build-system)
5. [Complete System Flow](#complete-system-flow)
6. [Deployment Guide](#deployment-guide)
7. [Troubleshooting](#troubleshooting)

---

## System Overview

This documentation covers the final integration layer of the telemetry logging system, bringing together all components into a deployable application.

**Components Covered:**
- `main.cpp`: Application entry point
- `config.json`: Runtime configuration
- `CMakeLists.txt`: Build system configuration
- Complete system startup and runtime behavior

**System Capabilities:**
- Multi-source telemetry collection (File, Socket, SOME/IP)
- Multi-sink output (Console, Multiple Files)
- Configurable buffering and threading
- Policy-based message formatting
- Graceful signal handling

---

## Main Application Entry Point

### main.cpp Analysis

```cpp
#include "LoggingApp.hpp"

int main() {
    // Create app with config path
    TelemetryLoggingApp app("/home/ayman/ITI/Project_cpp_iti/Phases/config.json");
    
    // Start all sources & writer thread
    app.start();
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### Execution Flow

```
┌─────────────────────────────────────────────────────────────┐
│                  main() Execution Flow                      │
└─────────────────────────────────────────────────────────────┘

1. PROGRAM START
   ┌──────────────────────────────────────────────┐
   │ int main()                                   │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ Create                │
              │ TelemetryLoggingApp   │
              │ with config path      │
              └──────────┬───────────┘
                         │
                         ├─→ Constructor executes:
                         │   1. loadConfig()
                         │   2. setupSinks()
                         │   3. Create LogManager
                         │   4. Transfer sinks
                         │   5. Register signals
                         │
                         ▼
              ┌──────────────────────┐
              │ app.start()          │
              └──────────┬───────────┘
                         │
                         ├─→ Start sequence:
                         │   1. isRunning = true
                         │   2. startWriterThread()
                         │   3. setupTelemetrySources()
                         │   4. join all threads
                         │
                         ▼
              ┌──────────────────────┐
              │ Background threads   │
              │ running:             │
              │ • Writer thread      │
              │ • File source thread │
              │ • Socket thread      │
              │ • SOME/IP thread     │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ Main thread enters   │
              │ infinite loop        │
              │ while(true)          │
              │   sleep(1s)          │
              └──────────┬───────────┘
                         │
                         │ Keeps process alive
                         │ until signal received
                         │
                         ▼
              ┌──────────────────────┐
              │ User presses Ctrl+C  │
              │ SIGINT received      │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │ signalHandler()      │
              │ isRunning = false    │
              │ exit(0)              │
              └──────────────────────┘
```

### Code Structure Analysis

#### Line-by-Line Breakdown

**Line 1: Include Header**
```cpp
#include "LoggingApp.hpp"
```
- Brings in `TelemetryLoggingApp` class definition
- Includes all necessary dependencies

**Line 3-5: Application Creation**
```cpp
TelemetryLoggingApp app("/home/ayman/ITI/Project_cpp_iti/Phases/config.json");
```

**What Happens During Construction:**
1. **Configuration Loading**:
   - Opens `config.json`
   - Parses JSON structure
   - Extracts parameters (buffer size, thread count, flush rate)

2. **Sink Setup**:
   - Creates ConsoleSinkImpl if enabled
   - Creates FileSinkImpl for each configured file
   - Stores in temporary vector

3. **LogManager Creation**:
   - Allocates circular buffer (200 entries)
   - Creates thread pool (4 threads)
   - Initializes internal structures

4. **Sink Transfer**:
   - Moves all sinks to LogManager ownership
   - LogManager now controls output destinations

5. **Signal Registration**:
   - Registers SIGINT handler (Ctrl+C)
   - Registers SIGTERM handler (kill command)
   - Sets global instance pointer

**Line 7-8: System Startup**
```cpp
app.start();
```

**What Happens During start():**

1. **Set Running Flag**:
   ```cpp
   isRunning = true;
   ```
   - Atomic flag for thread coordination

2. **Start Writer Thread**:
   ```cpp
   writerThread_ = std::thread([this]() {
       while (isRunning) {
           std::this_thread::sleep_for(std::chrono::milliseconds(500));
           logger->write();  // Flush every 500ms
       }
       logger->write();  // Final flush
   });
   ```
   - Independent thread for periodic flushing
   - Runs every 500ms (configurable)

3. **Setup Source Threads**:
   - Reads configuration for each source
   - Creates thread for enabled sources
   - Each thread runs independently

4. **Block Main Thread**:
   ```cpp
   for (auto &t : sourceThreads)
       if (t.joinable())
           t.join();
   ```
   - Main thread waits for source threads
   - Prevents premature program exit

**Line 10-12: Keep-Alive Loop**
```cpp
while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

**Purpose:**
- Keeps main thread alive
- Allows background threads to continue running
- Only exits on signal (Ctrl+C)

**Design Note:**
This infinite loop is intentional. Without it, the main thread would reach `return 0` and terminate the process, killing all background threads abruptly.

**Alternative Approaches:**

**Approach 1: Condition Variable (Better)**
```cpp
std::mutex cv_mutex;
std::condition_variable cv;
std::unique_lock<std::mutex> lock(cv_mutex);
cv.wait(lock, []{ return !app.isRunning; });
```

**Approach 2: Join Threads (Best)**
```cpp
// start() already does this internally
app.start();  // Blocks until all threads finish
```

**Approach 3: Signal-Based**
```cpp
std::atomic<bool> keep_running{true};
signal(SIGINT, [](int){ keep_running = false; });
while (keep_running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

### Thread Architecture Visualization

```
┌─────────────────────────────────────────────────────────────┐
│              Application Thread Architecture                │
└─────────────────────────────────────────────────────────────┘

Main Thread
     │
     ├─→ Construct TelemetryLoggingApp
     │   └─→ Load config, create LogManager
     │
     ├─→ Call app.start()
     │   │
     │   ├─→ Spawn Writer Thread ───────────┐
     │   │                                   │
     │   ├─→ Spawn SOME/IP Source Thread ───┤
     │   │                                   │
     │   └─→ Block on thread joins          │
     │                                       │
     │                                       ▼
     │                          ┌────────────────────────┐
     │                          │  Background Threads    │
     │                          ├────────────────────────┤
     │                          │                        │
     │                          │ [Writer Thread]        │
     │                          │  Loop every 500ms:     │
     │                          │    logger->write()     │
     │                          │                        │
     │                          │ [SOME/IP Thread]       │
     │                          │  Loop every 1200ms:    │
     │                          │    Read GPU data       │
     │                          │    Format with policy  │
     │                          │    logger->log()       │
     │                          │                        │
     │                          └────────────────────────┘
     │
     └─→ Infinite loop (sleep 1s)
          └─→ Keeps process alive


[User presses Ctrl+C]
         │
         ▼
    SIGINT signal
         │
         ▼
┌────────────────────┐
│  signalHandler()   │
│  isRunning = false │
│  exit(0)           │
└────────────────────┘
         │
         ▼
  All threads terminate
  Process exits
```

---

## Configuration File Structure

### Complete Configuration Analysis

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
      "enabled": false,
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
      "enabled": true,
      "parse_rate_ms": 1200,
      "policy": "cpu"
    }
  }
}
```

### Configuration Sections Deep Dive

#### Section 1: log_manager

```json
"log_manager": {
  "buffer_capacity": 200,
  "thread_pool_size": 4,
  "sink_flush_rate_ms": 500
}
```

**buffer_capacity: 200**
- **Purpose**: Size of lock-free circular buffer
- **Unit**: Number of LogMessage objects
- **Impact**: 
  - Higher value = More buffering, less message loss
  - Lower value = Less memory, faster overflow
- **Calculation**:
  ```
  Memory = buffer_capacity × sizeof(LogMessage)
         ≈ 200 × 200 bytes = 40 KB
  ```
- **Recommendation**: 
  - High-frequency logging: 500-1000
  - Normal usage: 200-500
  - Low-frequency: 50-200

**thread_pool_size: 4**
- **Purpose**: Number of worker threads for write operations
- **Impact**:
  - More threads = Better parallelism
  - Too many threads = Context switching overhead
- **Recommendation**: Match CPU core count
  ```
  Optimal = number_of_cores - 1
          (leave one for main thread)
  ```

**sink_flush_rate_ms: 500**
- **Purpose**: Milliseconds between sink flush operations
- **Trade-offs**:
  ```
  Lower (100-300ms):
    + Lower latency
    + Less data loss on crash
    - More I/O overhead
    - Higher CPU usage
  
  Higher (500-1000ms):
    + Better throughput
    + Less system calls
    - Higher latency
    - More data loss risk
  ```

#### Section 2: sinks

**Console Sink**
```json
"console": { "enabled": true }
```
- **enabled: true**: Logs written to stdout
- **Use Case**: Development, debugging, Docker containers
- **Output**: Terminal/console in real-time

**File Sinks**
```json
"files": [
  { 
    "enabled": true, 
    "path": "/home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log" 
  },
  { 
    "enabled": true, 
    "path": "/home/ayman/ITI/Project_cpp_iti/Phases/logs/backup.log" 
  }
]
```

**Multiple Files Rationale:**
1. **Primary Log** (`output.log`):
   - Main logging destination
   - Can be rotated or analyzed

2. **Backup Log** (`backup.log`):
   - Redundancy for critical data
   - Alternative if primary fails
   - Disaster recovery

**Path Requirements:**
- Directory must exist: `/home/ayman/ITI/Project_cpp_iti/Phases/logs/`
- Write permissions required
- Sufficient disk space

**Create Directory:**
```bash
mkdir -p /home/ayman/ITI/Project_cpp_iti/Phases/logs
chmod 755 /home/ayman/ITI/Project_cpp_iti/Phases/logs
```

#### Section 3: sources

**File Source (Currently Disabled)**
```json
"file": {
  "enabled": false,
  "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
  "parse_rate_ms": 1900,
  "policy": "cpu"
}
```

- **enabled: false**: Not active in current configuration
- **path**: Source file location
- **parse_rate_ms: 1900**: Read every 1.9 seconds
- **policy: "cpu"**: Uses CPU_policy formatter

**When to Enable:**
- Testing with pre-recorded data
- Replaying historical telemetry
- Development without live sources

**Socket Source (Currently Disabled)**
```json
"socket": {
  "enabled": false,
  "ip": "127.0.0.1",
  "port": 12345,
  "parse_rate_ms": 1500,
  "policy": "ram"
}
```

- **enabled: false**: Not active
- **ip: "127.0.0.1"**: Localhost (same machine)
- **port: 12345**: TCP port
- **parse_rate_ms: 1500**: Poll every 1.5 seconds
- **policy: "ram"**: Uses RAM_policy formatter

**Testing Socket Source:**
```bash
# Terminal 1: Start server
nc -lk 12345

# Terminal 2: Run application
./ITI_cpp

# Terminal 1: Type data
67.8
45.2
92.1
```

**SOME/IP Source (Currently Enabled)**
```json
"someip": {
  "enabled": true,
  "parse_rate_ms": 1200,
  "policy": "cpu"
}
```

- **enabled: true**: Active and running
- **parse_rate_ms: 1200**: Query every 1.2 seconds
- **policy: "cpu"**: Uses CPU_policy formatter
- **Note**: Requires SOME/IP service running

**SOME/IP Service Must Be Running:**
```bash
# Start SOME/IP GPU service
./server  # From same project
```

### Configuration Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│           Configuration Loading and Application             │
└─────────────────────────────────────────────────────────────┘

[config.json]
      │
      │ Loaded by TelemetryLoggingApp constructor
      │
      ▼
┌──────────────────────────────────────┐
│   loadConfig()                       │
│   Parse JSON structure               │
└──────────────┬───────────────────────┘
               │
               ├─→ log_manager section
               │   └─→ buffer_capacity = 200
               │   └─→ thread_pool_size = 4
               │   └─→ sink_flush_rate_ms = 500
               │
               ├─→ sinks section
               │   ├─→ console enabled
               │   └─→ 2 files enabled
               │       └─→ Create FileSinkImpl objects
               │
               └─→ sources section
                   ├─→ file: disabled (skip)
                   ├─→ socket: disabled (skip)
                   └─→ someip: enabled
                       └─→ Create source thread


Result:
┌────────────────────────────────┐
│ Active Components:             │
├────────────────────────────────┤
│ • LogManager (200 buffer, 4th) │
│ • ConsoleSink                  │
│ • FileSink (output.log)        │
│ • FileSink (backup.log)        │
│ • SOME/IP source thread        │
│ • Writer thread (500ms flush)  │
└────────────────────────────────┘
```

### Current System Configuration Summary

**What's Running:**
1. Writer thread flushing every 500ms
2. SOME/IP source thread reading GPU data every 1.2s
3. Console output (real-time)
4. Two file outputs (output.log, backup.log)
5. LogManager with 4-thread pool and 200-entry buffer

**What's Disabled:**
1. File source (local file reading)
2. Socket source (network telemetry)

---

## CMake Build System

### CMakeLists.txt Analysis

```cmake
cmake_minimum_required(VERSION 3.22)
project(ITI_cpp)

# CommonAPI dependencies
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)

# Include directories
include_directories(
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Include/
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen
)

# Collect source files
file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/sinks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/safe/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/telemetry/*.cpp
)

# Generated SOME/IP sources
set(GENERATED_SOMEIP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPDeployment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPProxy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPStubAdapter.cpp
)

# Main executable
add_executable(${PROJECT_NAME}
    app/main.cpp
    ${SRC_FILES}
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

# Server executable
add_executable(server
    app/server.cpp
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(server
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
```

### Build System Components

#### 1. Project Configuration

**Line 1: CMake Version**
```cmake
cmake_minimum_required(VERSION 3.22)
```
- Requires CMake 3.22 or higher
- Ensures modern CMake features available
- Version 3.22 released November 2021

**Line 2: Project Name**
```cmake
project(ITI_cpp)
```
- Project name: `ITI_cpp`
- Executable will be named `ITI_cpp`
- Used for target identification

#### 2. Dependency Management

**Lines 4-6: Find Packages**
```cmake
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)
```

**CommonAPI**
- Cross-platform C++ IPC framework
- Provides runtime environment
- Required for SOME/IP communication

**CommonAPI-SomeIP**
- SOME/IP binding for CommonAPI
- Automotive middleware standard
- Enables service-oriented communication

**vsomeip3**
- SOME/IP implementation by GENIVI
- Provides actual network layer
- Version 3.x (latest)

**Installation Check:**
```bash
# Verify packages installed
pkg-config --modversion CommonAPI
pkg-config --modversion CommonAPI-SomeIP
pkg-config --modversion vsomeip3
```

#### 3. Include Directories

**Lines 8-11: Header Paths**
```cmake
include_directories(
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Include/
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen
)
```

**Directory Structure:**
```
Project Root
├── Include/              ← Header files (.hpp)
│   ├── LoggingApp.hpp
│   ├── LogManager.hpp
│   ├── LogMessage.hpp
│   ├── sinks/
│   ├── safe/
│   └── telemetry/
│
└── gen_src/src-gen/      ← Generated SOME/IP code
    └── v1/omnimetron/gpu/
        ├── GpuUsageDataProxy.hpp
        └── ...
```

#### 4. Source File Collection

**Lines 13-18: Glob Patterns**
```cmake
file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/sinks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/safe/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/telemetry/*.cpp
)
```

**Collected Files:**
```
Source/
├── LogManager.cpp
├── LogMessage.cpp
├── LoggingApp.cpp
├── sinks/
│   ├── ConsoleSinkImpl.cpp
│   └── FileSinkImpl.cpp
├── safe/
│   ├── SafeFile.cpp
│   └── SafeSocket.cpp
└── telemetry/
    ├── FileTelemetrySourceImpl.cpp
    ├── SocketTelemetrySourceImpl.cpp
    └── SomeIPTelemetrySourceImpl.cpp
```

**Note on GLOB:**
- Convenient but can miss new files
- Better practice: Explicitly list files
- Requires CMake reconfiguration when files added

#### 5. Generated SOME/IP Code

**Lines 20-25: Generated Sources**
```cmake
set(GENERATED_SOMEIP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPDeployment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPProxy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPStubAdapter.cpp
)
```

**File Purposes:**
- **Deployment**: SOME/IP configuration and initialization
- **Proxy**: Client-side stub for GPU service
- **StubAdapter**: Server-side interface adapter

**Generation Process:**
```
FIDL definition (.fidl file)
        ↓
CommonAPI code generator
        ↓
Generated C++ code (.hpp, .cpp)
```

#### 6. Main Executable Target

**Lines 27-32: Client Application**
```cmake
add_executable(${PROJECT_NAME}
    app/main.cpp
    ${SRC_FILES}
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
```

**Build Output:**
- Executable name: `ITI_cpp`
- Entry point: `app/main.cpp`
- Includes all source files
- Links against SOME/IP libraries

#### 7. Server Executable Target

**Lines 34-42: Service Provider**
```cmake
add_executable(server
    app/server.cpp
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(server
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
```

**Build Output:**
- Executable name: `server`
- Provides GPU usage service
- Must run alongside main application

### Build Process Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                 CMake Build Process                         │
└─────────────────────────────────────────────────────────────┘

1. CONFIGURATION
   ┌──────────────────────────────────────┐
   │ cmake -S . -B build                  │
   └─────────────────┬────────────────────┘
                     │
                     ├─→ Find packages
                     │   ├─→ CommonAPI
                     │   ├─→ CommonAPI-SomeIP
                     │   └─→ vsomeip3
                     │
                     ├─→ Collect source files
                     │   └─→ file(GLOB ...)
                     │
                     └─→ Generate Makefiles
                         └─→ build/Makefile

2. COMPILATION
   ┌──────────────────────────────────────┐
   │ cmake --build build                  │
   └─────────────────┬────────────────────┘
                     │
                     ├─→ Compile .cpp → .o
                     │   ├─→ main.o
                     │   ├─→ LogManager.o
                     │   ├─→ LoggingApp.o
                     │   ├─→ ConsoleSinkImpl.o
                     │   └─→ ... (all sources)
                     │
                     ├─→ Link executable
                     │   └─→ ITI_cpp
                     │       (main + all .o + libs)
                     │
                     └─→ Link server
                         └─→ server
                             (server.o + generated + libs)

3. OUTPUT
   build/
   ├── ITI_cpp          ← Main application
   └── server           ← SOME/IP service
```

### Build Commands

**Full Build Sequence:**
```bash
# 1. Create build directory
mkdir -p build
cd build

# 2. Configure project
cmake ..

# 3. Build executables
cmake --build .

# 4. Executables ready
./ITI_cpp   # Main application
./server    # SOME/IP service
```

**Clean and Rebuild:**
```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

**Build with Verbose Output:**
```bash
cmake --build build --verbose
```

---

## Complete System Flow

### Startup Sequence

```
┌─────────────────────────────────────────────────────────────┐
│           Complete Application Startup Flow                 │
└─────────────────────────────────────────────────────────────┘

STEP 1: PROCESS START
┌──────────────────────────┐
│ ./ITI_cpp                │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────────────┐
│ main() executed          │
└────────┬─────────────────┘
         │
         ▼

STEP 2: CONFIGURATION LOADING
┌────────────────────────────────────────────┐
│ TelemetryLoggingApp app("config.json")     │
└────────┬───────────────────────────────────┘
         │
         ├─→ loadConfig("config.json")
         │   ├─→ Open file
         │   ├─→ Parse JSON
         │   └─→ Extract parameters
         │       ├─→ buffer_capacity = 200
         │       ├─→ thread_pool_size = 4
         │       └─→ sink_flush_rate_ms = 500
         │
         ▼

STEP 3: SINK INITIALIZATION
┌────────────────────────────────────────────┐
│ setupSinks()                               │
└────────┬───────────────────────────────────┘
         │
         ├─→ Console enabled → Create ConsoleSinkImpl
         │
         ├─→ File 1 enabled → Create FileSinkImpl("output.log")
         │
         └─→ File 2 enabled → Create FileSinkImpl("backup.log")
         │
         ▼

STEP 4: LOGMANAGER CREATION
┌────────────────────────────────────────────┐
│ logger = make_unique<LogManager>(200, 4)   │
└────────┬───────────────────────────────────┘
         │
         ├─→ Create circular buffer (200 capacity)
         ├─→ Create thread pool (4 threads)
         └─→ Initialize internal structures
         │
         ▼

STEP 5: SINK TRANSFER
┌────────────────────────────────────────────┐
│ for (auto &sink : sinks)                   │
│     logger->add_sink(std::move(sink))      │
└────────┬───────────────────────────────────┘
         │
         ├─→ Move ConsoleSink to LogManager
         ├─→ Move FileSink 1 to LogManager
         └─→ Move FileSink 2 to LogManager
         │
         ▼

STEP 6: SIGNAL REGISTRATION
┌────────────────────────────────────────────┐
│ signal(SIGINT, signalHandler)              │
│ signal(SIGTERM, signalHandler)             │
└────────┬───────────────────────────────────┘
         │
         ├─→ Register Ctrl+C handler
         └─→ Register kill handler
         │
         ▼

STEP 7: APPLICATION START
┌────────────────────────────────────────────┐
│ app.start()                                │
└────────┬───────────────────────────────────┘
         │
         ├─→ isRunning = true
         │
         ├─→ startWriterThread()
         │   └─→ Spawn thread: flush every 500ms
         │
         ├─→ setupTelemetrySources()
         │   └─→ SOME/IP enabled
         │       └─→ Spawn thread: read GPU every 1200ms
         │
         └─→ join threads (blocks main)
         │
         ▼

STEP 8: RUNTIME OPERATION
┌────────────────────────────────────────────┐
│ System Running                             │
├────────────────────────────────────────────┤
│                                            │
│ [Writer Thread]                            │
│   while(isRunning)                         │
│     sleep(500ms)                           │
│     logger->write() ───┐                   │
│                        │                   │
│ [SOME/IP Thread]       │                   │
│   while(isRunning)     │                   │
│     sleep(1200ms)      │                   │
│     Read GPU data      │                   │
│     Format message     │                   │
│     logger->log() ─────┤                   │
│                        │                   │
│ [LogManager]           │                   │
│   Circular Buffer  ←───┘                   │
│   Thread Pool                              │
│                        │                   │
│ [Sinks]                │                   │
│   Console      ←───────┘                   │
│   output.log   ←───────┘                   │
│   backup.log   ←───────┘                   │
│                                            │
└────────────────────────────────────────────┘

STEP 9: SIGNAL RECEIVED
┌────────────────────────────────────────────┐
│ User presses Ctrl+C                        │
└────────┬───────────────────────────────────┘
         │
         ▼
┌────────────────────────────────────────────┐
│ signalHandler(SIGINT)                      │
│   isRunning = false                        │
│   exit(0)                                  │
└────────┬───────────────────────────────────┘
         │
         ▼

STEP 10: CLEANUP
┌────────────────────────────────────────────┐
│ Destructor chain                           │
├────────────────────────────────────────────┤
│ ~TelemetryLoggingApp()                     │
│   └─→ isRunning = false                    │
│   └─→ Join all threads                     │
│                                            │
│ ~LogManager()                              │
│   └─→ Destroy buffer                       │
│   └─→ Destroy thread pool                  │
│                                            │
│ ~ConsoleSinkImpl(), ~FileSinkImpl()        │
│   └─→ Close file descriptors               │
│                                            │
│ ~SafeFile(), ~SafeSocket()                 │
│   └─→ RAII cleanup                         │
└────────────────────────────────────────────┘
```

### Runtime Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│              Runtime Telemetry Data Flow                    │
└─────────────────────────────────────────────────────────────┘

[SOME/IP GPU Service]
         │
         │ Publishes GPU usage data
         │
         ▼
┌──────────────────────┐
│ SomeIPTelemetrySrc   │
│ readSource()         │
└────────┬─────────────┘
         │
         │ raw = "67.3"
         │
         ▼
┌──────────────────────┐
│ Formatter<CPU_policy>│
│ format(raw)          │
└────────┬─────────────┘
         │
         │ LogMessage created
         │
         ▼
┌──────────────────────┐
│ logger->log(msg)     │
└────────┬─────────────┘
         │
         ▼
┌──────────────────────┐
│ LockFreeCircular     │
│ Buffer               │
│ [msg1][msg2][msg3]   │
└────────┬─────────────┘
         │
         │ Every 500ms
         │
         ▼
┌──────────────────────┐
│ Writer Thread        │
│ logger->write()      │
└────────┬─────────────┘
         │
         │ Pop messages
         │
         ▼
┌──────────────────────────────────┐
│ For each sink:                   │
│   sink->write(msg)               │
└────────┬─────────────────────────┘
         │
         ├──────────────┬──────────────────┐
         │              │                  │
         ▼              ▼                  ▼
   ┌─────────┐  ┌────────────┐  ┌────────────┐
   │ Console │  │output.log  │  │backup.log  │
   └─────────┘  └────────────┘  └────────────┘
```

---

## Deployment Guide

### Prerequisites

**System Requirements:**
- Ubuntu 20.04+ or similar Linux distribution
- CMake 3.22+
- GCC 9+ or Clang 10+
- CommonAPI runtime
- SOME/IP middleware (vsomeip3)

**Install Dependencies:**
```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install build-essential cmake

# Install CommonAPI dependencies (if not already installed)
sudo apt install libcommonapi-dev libcommonapi-someip-dev libvsomeip3-dev

# Verify installations
cmake --version
g++ --version
pkg-config --modversion CommonAPI
```

### Build Steps

```bash
# 1. Clone or navigate to project
cd /home/ayman/ITI/Project_cpp_iti/Phases

# 2. Create build directory
mkdir -p build
cd build

# 3. Configure project
cmake ..

# 4. Build (parallel build with 4 jobs)
cmake --build . -j4

# 5. Verify executables
ls -lh ITI_cpp server
```

### Directory Setup

```bash
# Create necessary directories
mkdir -p /home/ayman/ITI/Project_cpp_iti/Phases/logs
mkdir -p /home/ayman/ITI/Project_cpp_iti/Phases/scripts

# Set permissions
chmod 755 /home/ayman/ITI/Project_cpp_iti/Phases/logs
```

### Running the System

**Two-Terminal Setup:**

**Terminal 1: Start SOME/IP Service**
```bash
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./server

# Expected output:
# SOME/IP service started
# Waiting for clients...
```

**Terminal 2: Start Main Application**
```bash
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp

# Expected output:
# [TelemetryApp] [timestamp] [SOMEIP] [INFO] [GPU Usage: 67.3%]
# [TelemetryApp] [timestamp] [SOMEIP] [INFO] [GPU Usage: 68.1%]
# ...
```

**Stop Application:**
- Press `Ctrl+C` in main application terminal
- Server can continue running for other clients

### Verification

**Check Log Files:**
```bash
# View output log
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# View backup log
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/backup.log

# Check file sizes
ls -lh /home/ayman/ITI/Project_cpp_iti/Phases/logs/
```

**Expected Log Format:**
```
[TelemetryApp] [2025-02-21 14:30:45] [SOMEIP] [INFO] [GPU Usage: 67.3%]
[TelemetryApp] [2025-02-21 14:30:46] [SOMEIP] [INFO] [GPU Usage: 68.1%]
[TelemetryApp] [2025-02-21 14:30:48] [SOMEIP] [INFO] [GPU Usage: 65.9%]
```

### Configuration Changes

**Enable File Source:**
```json
"file": {
  "enabled": true,
  "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
  "parse_rate_ms": 1900,
  "policy": "cpu"
}
```

**Create test file:**
```bash
echo "45.2" > /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt
echo "67.8" >> /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt
echo "92.1" >> /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt
```

**Enable Socket Source:**
```json
"socket": {
  "enabled": true,
  "ip": "127.0.0.1",
  "port": 12345,
  "parse_rate_ms": 1500,
  "policy": "ram"
}
```

**Start test server:**
```bash
# Terminal 3
nc -lk 12345

# Type telemetry data:
78.5
82.3
91.0
```

---

## Troubleshooting

### Common Issues

#### Issue 1: Config File Not Found

**Error:**
```
terminate called after throwing an instance of 'std::runtime_error'
  what():  Cannot open config: /path/to/config.json
```

**Solution:**
```bash
# Verify file exists
ls -l /home/ayman/ITI/Project_cpp_iti/Phases/config.json

# Check current directory
pwd

# Run from correct directory or use absolute path
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp
```

#### Issue 2: Log Directory Doesn't Exist

**Error:**
```
Failed to open log file: /path/to/logs/output.log
```

**Solution:**
```bash
mkdir -p /home/ayman/ITI/Project_cpp_iti/Phases/logs
chmod 755 /home/ayman/ITI/Project_cpp_iti/Phases/logs
```

#### Issue 3: SOME/IP Connection Failed

**Error:**
```
Failed to connect to SOME/IP service
```

**Solution:**
```bash
# Check if server is running
ps aux | grep server

# Start server first
./server

# Then start client
./ITI_cpp
```

#### Issue 4: Build Fails - Missing Dependencies

**Error:**
```
CMake Error: Could not find a package configuration file provided by "CommonAPI"
```

**Solution:**
```bash
# Install CommonAPI packages
sudo apt install libcommonapi-dev libcommonapi-someip-dev libvsomeip3-dev

# Reconfigure and rebuild
cd build
rm -rf *
cmake ..
cmake --build .
```

#### Issue 5: Permission Denied on Log Files

**Error:**
```
Permission denied: /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log
```

**Solution:**
```bash
# Fix permissions
chmod 755 /home/ayman/ITI/Project_cpp_iti/Phases/logs
chmod 644 /home/ayman/ITI/Project_cpp_iti/Phases/logs/*.log

# Or delete and recreate
rm -rf /home/ayman/ITI/Project_cpp_iti/Phases/logs
mkdir -p /home/ayman/ITI/Project_cpp_iti/Phases/logs
```

### Debug Mode

**Enable Verbose Logging:**
Modify main.cpp temporarily:
```cpp
int main() {
    std::cout << "Starting application...\n";
    std::cout << "Config path: " << CONFIG_PATH << "\n";
    
    try {
        TelemetryLoggingApp app(CONFIG_PATH);
        std::cout << "App created successfully\n";
        
        app.start();
        std::cout << "App started\n";
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### System Monitoring

**Monitor Resource Usage:**
```bash
# CPU and memory
top -p $(pgrep ITI_cpp)

# Thread count
ps -eLf | grep ITI_cpp

# Open file descriptors
lsof -p $(pgrep ITI_cpp)

# Log file growth
watch -n 1 ls -lh /home/ayman/ITI/Project_cpp_iti/Phases/logs/
```

---

## Summary

### System Architecture Overview

The complete telemetry logging system consists of:

**Core Components:**
1. TelemetryLoggingApp - Top-level orchestrator
2. LogManager - Message buffering and routing
3. Sources - Data acquisition (File, Socket, SOME/IP)
4. Sinks - Output destinations (Console, Files)
5. Formatters - Policy-based message formatting

**Key Features:**
- Multi-threaded architecture
- Lock-free circular buffer
- Configurable via JSON
- RAII resource management
- Signal-safe shutdown
- Multiple input and output channels

**Current Configuration:**
- Buffer: 200 messages
- Thread pool: 4 workers
- Flush rate: 500ms
- Active source: SOME/IP (GPU data every 1.2s)
- Active sinks: Console + 2 files

