# Telemetry Logging System - Complete Project Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Component Breakdown](#component-breakdown)
4. [Data Flow Visualization](#data-flow-visualization)
5. [Installation and Setup](#installation-and-setup)
6. [Running Each Source Type](#running-each-source-type)
7. [Configuration Guide](#configuration-guide)
8. [Terminal Usage Examples](#terminal-usage-examples)
9. [Complete Usage Scenarios](#complete-usage-scenarios)
10. [Project Structure](#project-structure)

---

## Project Overview

### What is This Project?

This is a **multi-threaded telemetry logging system** designed to collect data from various sources (files, network sockets, automotive middleware), process it, and output it to multiple destinations (console, log files).

**Real-World Use Case:**
Imagine a car with multiple sensors (CPU usage, RAM usage, GPU usage). This system:
- Collects data from these sensors
- Formats the data into readable log messages
- Stores logs in files for later analysis
- Displays real-time logs on screen for monitoring

### Key Features

```
┌─────────────────────────────────────────────────────────────┐
│                    System Capabilities                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  INPUT FLEXIBILITY                                          │
│  • Read from files (testing/replay)                         │
│  • Read from network sockets (remote monitoring)            │
│  • Read from SOME/IP services (automotive systems)          │
│                                                             │
│  PROCESSING POWER                                           │
│  • Multi-threaded architecture                              │
│  • Lock-free circular buffer (high performance)             │
│  • Configurable thread pool                                 │
│  • Policy-based message formatting                          │
│                                                             │
│  OUTPUT VERSATILITY                                         │
│  • Real-time console output                                 │
│  • Multiple log files (primary + backup)                    │
│  • Extensible sink architecture                             │
│                                                             │
│  RELIABILITY                                                │
│  • RAII resource management (no leaks)                      │
│  • Graceful signal handling                                 │
│  • Thread-safe operations                                   │
│  • Exception-safe design                                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Technologies Used

| Technology | Purpose | Version |
|------------|---------|---------|
| **C++17** | Core language | Standard |
| **CMake** | Build system | 3.22+ |
| **CommonAPI** | IPC framework | Latest |
| **SOME/IP** | Automotive middleware | vsomeip3 |
| **JSON** | Configuration | nlohmann/json |
| **POSIX Threads** | Threading | pthread |

---

## System Architecture

### Big Picture - 10,000 Foot View

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     TELEMETRY LOGGING SYSTEM                                │
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                        INPUT LAYER                                 │    │
│  │                     (Data Acquisition)                             │    │
│  ├────────────────────────────────────────────────────────────────────┤    │
│  │                                                                    │    │
│  │   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │    │
│  │   │ File Source  │    │Socket Source │    │SOME/IP Source│       │    │
│  │   │              │    │              │    │              │       │    │
│  │   │ Read from    │    │ TCP network  │    │ Automotive   │       │    │
│  │   │ local files  │    │ connections  │    │ middleware   │       │    │
│  │   └──────┬───────┘    └──────┬───────┘    └──────┬───────┘       │    │
│  │          │                   │                   │               │    │
│  │          └───────────────────┼───────────────────┘               │    │
│  │                              │                                   │    │
│  └──────────────────────────────┼───────────────────────────────────┘    │
│                                 │                                        │
│                                 │ Raw telemetry strings                  │
│                                 │ "45.2", "67.8", etc.                   │
│                                 │                                        │
│  ┌──────────────────────────────┼───────────────────────────────────┐    │
│  │                        PROCESSING LAYER                           │    │
│  │                     (Formatting & Buffering)                      │    │
│  ├──────────────────────────────┼───────────────────────────────────┤    │
│  │                              ▼                                    │    │
│  │                   ┌────────────────────┐                          │    │
│  │                   │    Formatters      │                          │    │
│  │                   │                    │                          │    │
│  │                   │  CPU_policy        │                          │    │
│  │                   │  RAM_policy        │                          │    │
│  │                   │  GPU_policy        │                          │    │
│  │                   └──────────┬─────────┘                          │    │
│  │                              │                                    │    │
│  │                              │ Formatted LogMessage               │    │
│  │                              │ [app][time][context][level][msg]   │    │
│  │                              │                                    │    │
│  │                              ▼                                    │    │
│  │                   ┌────────────────────┐                          │    │
│  │                   │   LogManager       │                          │    │
│  │                   ├────────────────────┤                          │    │
│  │                   │ Circular Buffer    │                          │    │
│  │                   │ [msg][msg][msg]... │                          │    │
│  │                   │                    │                          │    │
│  │                   │ Thread Pool        │                          │    │
│  │                   │ [T1][T2][T3][T4]   │                          │    │
│  │                   └──────────┬─────────┘                          │    │
│  │                              │                                    │    │
│  └──────────────────────────────┼───────────────────────────────────┘    │
│                                 │                                        │
│                                 │ Batched messages                       │
│                                 │ (flushed every 500ms)                  │
│                                 │                                        │
│  ┌──────────────────────────────┼───────────────────────────────────┐    │
│  │                        OUTPUT LAYER                               │    │
│  │                      (Data Persistence)                           │    │
│  ├──────────────────────────────┼───────────────────────────────────┤    │
│  │                              ▼                                    │    │
│  │                   ┌────────────────────┐                          │    │
│  │                   │   Write to Sinks   │                          │    │
│  │                   └──────────┬─────────┘                          │    │
│  │                              │                                    │    │
│  │          ┌───────────────────┼───────────────────┐                │    │
│  │          │                   │                   │                │    │
│  │          ▼                   ▼                   ▼                │    │
│  │   ┌────────────┐      ┌────────────┐     ┌────────────┐          │    │
│  │   │  Console   │      │ output.log │     │backup.log  │          │    │
│  │   │  (stdout)  │      │  (file)    │     │  (file)    │          │    │
│  │   └────────────┘      └────────────┘     └────────────┘          │    │
│  │        │                    │                   │                 │    │
│  │        ▼                    ▼                   ▼                 │    │
│  │   [Terminal]          [Disk Storage]      [Backup Storage]       │    │
│  │                                                                   │    │
│  └───────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Threading Model

```
┌─────────────────────────────────────────────────────────────┐
│                  Thread Architecture                        │
└─────────────────────────────────────────────────────────────┘

┌──────────────────┐
│   Main Thread    │
└────────┬─────────┘
         │
         ├─→ Creates TelemetryLoggingApp
         │   └─→ Loads config, sets up sinks
         │
         ├─→ Calls app.start()
         │   │
         │   ├───────────────────────────────────────────────┐
         │   │                                               │
         │   ▼                                               │
         │  ┌─────────────────────────┐                      │
         │  │  Writer Thread          │                      │
         │  ├─────────────────────────┤                      │
         │  │ while(isRunning) {      │                      │
         │  │   sleep(500ms)          │                      │
         │  │   logger->write()       │                      │
         │  │   // Flush to sinks     │                      │
         │  │ }                       │                      │
         │  └─────────────────────────┘                      │
         │                                                   │
         │   ▼                                               │
         │  ┌─────────────────────────┐                      │
         │  │  File Source Thread     │                      │
         │  ├─────────────────────────┤                      │
         │  │ while(isRunning) {      │                      │
         │  │   sleep(1900ms)         │                      │
         │  │   readSource(data)      │                      │
         │  │   format(data)          │                      │
         │  │   logger->log(msg)      │                      │
         │  │ }                       │                      │
         │  └─────────────────────────┘                      │
         │                                                   │
         │   ▼                                               │
         │  ┌─────────────────────────┐                      │
         │  │ Socket Source Thread    │                      │
         │  ├─────────────────────────┤                      │
         │  │ while(isRunning) {      │                      │
         │  │   sleep(1500ms)         │                      │
         │  │   readSource(data)      │                      │
         │  │   format(data)          │                      │
         │  │   logger->log(msg)      │                      │
         │  │ }                       │                      │
         │  └─────────────────────────┘                      │
         │                                                   │
         │   ▼                                               │
         │  ┌─────────────────────────┐                      │
         │  │ SOME/IP Source Thread   │                      │
         │  ├─────────────────────────┤                      │
         │  │ while(isRunning) {      │                      │
         │  │   sleep(1200ms)         │                      │
         │  │   readSource(data)      │                      │
         │  │   format(data)          │                      │
         │  │   logger->log(msg)      │                      │
         │  │ }                       │                      │
         │  └─────────────────────────┘                      │
         │                                                   │
         └───────────────────────────────────────────────────┘
         │
         └─→ Infinite loop (keeps process alive)
             └─→ while(true) sleep(1s)

All threads share the same LogManager and circular buffer
(thread-safe via lock-free operations)
```

---

## Component Breakdown

### Layer 1: Data Sources

```
┌─────────────────────────────────────────────────────────────┐
│                    SOURCE COMPONENTS                        │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 1. FileTelemetrySrc                                        │
├────────────────────────────────────────────────────────────┤
│ Purpose: Read telemetry from files line-by-line           │
│                                                            │
│ Use Cases:                                                 │
│  • Testing with static datasets                           │
│  • Replaying recorded sessions                            │
│  • Simulating sensor data                                 │
│                                                            │
│ How it works:                                              │
│  1. Open file at specified path                           │
│  2. Read one line at a time                               │
│  3. Pass to formatter                                      │
│  4. Sleep for configured interval                          │
│  5. Repeat until EOF                                       │
│                                                            │
│ Example file content:                                      │
│   45.2                                                     │
│   67.8                                                     │
│   92.1                                                     │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 2. SocketTelemetrySrc                                      │
├────────────────────────────────────────────────────────────┤
│ Purpose: Receive telemetry over TCP network               │
│                                                            │
│ Use Cases:                                                 │
│  • Remote sensor monitoring                               │
│  • Distributed systems                                     │
│  • Network-connected devices                              │
│                                                            │
│ How it works:                                              │
│  1. Connect to server at IP:Port                          │
│  2. Receive data line-by-line                             │
│  3. Pass to formatter                                      │
│  4. Sleep for configured interval                          │
│  5. Reconnect if connection lost                           │
│                                                            │
│ Network protocol:                                          │
│   Server sends: "45.2\n"                                   │
│   Client receives: "45.2"                                  │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 3. SomeIPTelemetrySourceImpl                               │
├────────────────────────────────────────────────────────────┤
│ Purpose: Integrate with automotive middleware              │
│                                                            │
│ Use Cases:                                                 │
│  • Automotive ECU communication                           │
│  • Service-oriented architectures                         │
│  • Event-driven telemetry                                 │
│                                                            │
│ How it works:                                              │
│  1. Connect to SOME/IP service                            │
│  2. Subscribe to GPU usage events                         │
│  3. Request current values periodically                    │
│  4. Pass to formatter                                      │
│  5. Sleep for configured interval                          │
│                                                            │
│ Communication model:                                       │
│   • Request-Response: Query current value                 │
│   • Event-Based: Receive notifications on change          │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Layer 2: Message Processing

```
┌─────────────────────────────────────────────────────────────┐
│                  PROCESSING COMPONENTS                      │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 1. Formatters (Policy-Based)                               │
├────────────────────────────────────────────────────────────┤
│                                                            │
│ CPU_policy:                                                │
│   Input:  "45.2"                                           │
│   Output: LogMessage(                                      │
│             app_name = "TelemetryApp",                     │
│             context  = "CPU",                              │
│             message  = "CPU Usage: 45.2%",                 │
│             level    = INFO,                               │
│             time     = "2025-02-21 14:30:45"               │
│           )                                                │
│                                                            │
│ RAM_policy:                                                │
│   Input:  "67.8"                                           │
│   Output: LogMessage(                                      │
│             app_name = "TelemetryApp",                     │
│             context  = "RAM",                              │
│             message  = "RAM Usage: 67.8%",                 │
│             level    = INFO,                               │
│             time     = "2025-02-21 14:30:45"               │
│           )                                                │
│                                                            │
│ GPU_policy:                                                │
│   Input:  "92.1"                                           │
│   Output: LogMessage(                                      │
│             app_name = "TelemetryApp",                     │
│             context  = "GPU",                              │
│             message  = "GPU Usage: 92.1%",                 │
│             level    = INFO,                               │
│             time     = "2025-02-21 14:30:45"               │
│           )                                                │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 2. LogManager (Central Hub)                                │
├────────────────────────────────────────────────────────────┤
│                                                            │
│ Components:                                                │
│  ┌──────────────────────────────────────┐                 │
│  │ Lock-Free Circular Buffer            │                 │
│  │ ┌────┐┌────┐┌────┐┌────┐┌────┐      │                 │
│  │ │msg1││msg2││msg3││msg4││msg5│...    │                 │
│  │ └────┘└────┘└────┘└────┘└────┘      │                 │
│  │ Capacity: 200 messages               │                 │
│  └──────────────────────────────────────┘                 │
│                                                            │
│  ┌──────────────────────────────────────┐                 │
│  │ Thread Pool                          │                 │
│  │ ┌────────┐┌────────┐┌────────┐      │                 │
│  │ │Worker 1││Worker 2││Worker 3│...    │                 │
│  │ └────────┘└────────┘└────────┘      │                 │
│  │ Size: 4 threads                      │                 │
│  └──────────────────────────────────────┘                 │
│                                                            │
│ Operations:                                                │
│  • log(message) → Push to buffer                          │
│  • write() → Pop from buffer, send to sinks               │
│  • add_sink() → Register output destination               │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Layer 3: Output Destinations

```
┌─────────────────────────────────────────────────────────────┐
│                     SINK COMPONENTS                         │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 1. ConsoleSinkImpl                                         │
├────────────────────────────────────────────────────────────┤
│ Purpose: Display logs in terminal                          │
│                                                            │
│ Output format:                                             │
│   [TelemetryApp] [2025-02-21 14:30:45] [CPU] [INFO]       │
│   [CPU Usage: 45.2%]                                       │
│                                                            │
│ Characteristics:                                           │
│  • Real-time feedback                                     │
│  • No persistence (lost after exit)                       │
│  • Ideal for development/debugging                        │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ 2. FileSinkImpl                                            │
├────────────────────────────────────────────────────────────┤
│ Purpose: Write logs to disk files                         │
│                                                            │
│ Output location:                                           │
│   /home/ayman/ITI/.../logs/output.log                     │
│   /home/ayman/ITI/.../logs/backup.log                     │
│                                                            │
│ File content example:                                      │
│   [TelemetryApp] [2025-02-21 14:30:45] [CPU] [INFO]       │
│   [CPU Usage: 45.2%]                                       │
│   [TelemetryApp] [2025-02-21 14:30:46] [RAM] [INFO]       │
│   [RAM Usage: 67.8%]                                       │
│                                                            │
│ Characteristics:                                           │
│  • Persistent storage                                     │
│  • Post-mortem debugging                                  │
│  • Audit trails                                           │
│  • Multiple files for redundancy                          │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## Data Flow Visualization

### Complete End-to-End Flow

```
┌───────────────────────────────────────────────────────────────────────────┐
│              TELEMETRY DATA JOURNEY (Step-by-Step)                        │
└───────────────────────────────────────────────────────────────────────────┘

STEP 1: DATA GENERATION
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────┐
│ Data Source │  File: "45.2\n67.8\n92.1\n"
│             │  Socket: Server sends "45.2\n"
│             │  SOME/IP: GPU service publishes 45.2
└──────┬──────┘
       │
       │ Raw telemetry string
       │
       ▼

STEP 2: SOURCE READING
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────────────┐
│ Source Thread       │
│ (File/Socket/SOME)  │
└──────┬──────────────┘
       │
       │ readSource(raw)
       │ → raw = "45.2"
       │
       ▼

STEP 3: FORMATTING
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────────────┐
│ Formatter           │
│ <CPU_policy>        │
└──────┬──────────────┘
       │
       │ format("45.2")
       │
       │ Creates LogMessage:
       │  ┌─────────────────────────────────┐
       │  │ app_name: "TelemetryApp"        │
       │  │ time:     "2025-02-21 14:30:45" │
       │  │ context:  "CPU"                 │
       │  │ level:    INFO                  │
       │  │ message:  "CPU Usage: 45.2%"    │
       │  └─────────────────────────────────┘
       │
       ▼

STEP 4: LOGGING
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────────────┐
│ logger->log(msg)    │
└──────┬──────────────┘
       │
       │ tryPush to buffer
       │
       ▼
┌──────────────────────────────────┐
│ Circular Buffer                  │
│ ┌─────┐┌─────┐┌─────┐┌─────┐    │
│ │msg1 ││msg2 ││msg3 ││     │... │
│ └─────┘└─────┘└─────┘└─────┘    │
│                                  │
│ Thread-safe lock-free queue      │
└──────┬───────────────────────────┘
       │
       │ Messages accumulate
       │
       ▼

STEP 5: BATCHING
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────────────┐
│ Writer Thread       │
│ (runs every 500ms)  │
└──────┬──────────────┘
       │
       │ logger->write()
       │
       │ Pop all messages from buffer
       │
       ▼

STEP 6: DISTRIBUTION
━━━━━━━━━━━━━━━━━━━━━━
┌─────────────────────┐
│ For each sink:      │
│   sink->write(msg)  │
└──────┬──────────────┘
       │
       ├────────────────┬────────────────┐
       │                │                │
       ▼                ▼                ▼
┌──────────┐   ┌──────────────┐  ┌──────────────┐
│ Console  │   │  output.log  │  │  backup.log  │
└────┬─────┘   └──────┬───────┘  └──────┬───────┘
     │                │                 │
     ▼                ▼                 ▼

STEP 7: OUTPUT
━━━━━━━━━━━━━━━━━━━━━━
Terminal:                    File Contents:
┌──────────────────────┐     ┌──────────────────────┐
│ [TelemetryApp]       │     │ [TelemetryApp]       │
│ [14:30:45]           │     │ [14:30:45]           │
│ [CPU] [INFO]         │     │ [CPU] [INFO]         │
│ [CPU Usage: 45.2%]   │     │ [CPU Usage: 45.2%]   │
│                      │     │                      │
│ [TelemetryApp]       │     │ [TelemetryApp]       │
│ [14:30:46]           │     │ [14:30:46]           │
│ [RAM] [INFO]         │     │ [RAM] [INFO]         │
│ [RAM Usage: 67.8%]   │     │ [RAM Usage: 67.8%]   │
└──────────────────────┘     └──────────────────────┘
```

### Timing Diagram

```
┌───────────────────────────────────────────────────────────────┐
│                   System Timing Diagram                       │
└───────────────────────────────────────────────────────────────┘

Time ──────────────────────────────────────────────────────────→

0ms
│
├─ [App Start]
│  └─→ All threads spawned
│
500ms
│
├─ [Writer Thread]
│  └─→ First flush to sinks
│
1200ms
│
├─ [SOME/IP Thread]
│  └─→ First GPU read
│
1500ms
│
├─ [Socket Thread] (if enabled)
│  └─→ First socket read
│
1900ms
│
├─ [File Thread] (if enabled)
│  └─→ First file line read
│
2000ms
│
├─ [Writer Thread]
│  └─→ Second flush
│
2400ms
│
├─ [SOME/IP Thread]
│  └─→ Second GPU read
│
3000ms
│
├─ [Writer Thread]
│  └─→ Third flush
│
(continues indefinitely...)

Note: All timings are approximate and depend on configuration.
```

---

## Installation and Setup

### Prerequisites

```
┌─────────────────────────────────────────────────────────────┐
│                  System Requirements                        │
└─────────────────────────────────────────────────────────────┘

Operating System:
  ✓ Ubuntu 20.04 or later
  ✓ Any Linux with glibc 2.27+

Build Tools:
  ✓ CMake 3.22 or later
  ✓ GCC 9+ or Clang 10+
  ✓ Make or Ninja

Libraries:
  ✓ CommonAPI (IPC framework)
  ✓ CommonAPI-SomeIP (SOME/IP binding)
  ✓ vsomeip3 (SOME/IP implementation)
  ✓ nlohmann-json (JSON parsing)
  ✓ pthread (POSIX threads)

Optional:
  ✓ netcat (for socket testing)
  ✓ gdb (for debugging)
```

### Installation Steps

```bash
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STEP 1: Update System
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
sudo apt update
sudo apt upgrade -y

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STEP 2: Install Build Tools
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STEP 3: Install Dependencies
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
sudo apt install -y \
    libcommonapi-dev \
    libcommonapi-someip-dev \
    libvsomeip3-dev \
    nlohmann-json3-dev

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STEP 4: Verify Installation
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
cmake --version       # Should show 3.22 or higher
g++ --version         # Should show GCC 9 or higher
pkg-config --modversion CommonAPI
pkg-config --modversion vsomeip3
```

### Project Setup

```bash
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Navigate to project directory
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
cd /home/ayman/ITI/Project_cpp_iti/Phases

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Create necessary directories
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
mkdir -p logs
mkdir -p scripts
mkdir -p build

# Set permissions
chmod 755 logs scripts

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Build the project
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
cd build
cmake ..
cmake --build . -j$(nproc)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Verify build output
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
ls -lh ITI_cpp server

# Expected output:
# -rwxr-xr-x 1 user user 2.3M Feb 21 14:30 ITI_cpp
# -rwxr-xr-x 1 user user 1.8M Feb 21 14:30 server
```

### Directory Structure After Setup

```
/home/ayman/ITI/Project_cpp_iti/Phases/
│
├── build/
│   ├── ITI_cpp          ← Main executable
│   └── server           ← SOME/IP service executable
│
├── logs/
│   ├── output.log       ← Created at runtime
│   └── backup.log       ← Created at runtime
│
├── scripts/
│   └── shell_logs.txt   ← Sample data file (create manually)
│
├── config.json          ← Configuration file
│
├── app/
│   ├── main.cpp
│   └── server.cpp
│
├── Source/
│   ├── LogManager.cpp
│   ├── LogMessage.cpp
│   ├── LoggingApp.cpp
│   ├── sinks/
│   ├── safe/
│   └── telemetry/
│
├── Include/
│   └── (all .hpp files)
│
└── CMakeLists.txt
```

---

## Running Each Source Type

### Scenario 1: Running with File Source

```
┌─────────────────────────────────────────────────────────────┐
│              FILE SOURCE CONFIGURATION                      │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ STEP 1: Create Test Data File                             │
└────────────────────────────────────────────────────────────┘

# Create sample telemetry data
cat > /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt << 'EOF'
45.2
47.8
50.1
52.3
48.9
51.2
EOF

# Verify file created
cat /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt


┌────────────────────────────────────────────────────────────┐
│ STEP 2: Configure config.json                             │
└────────────────────────────────────────────────────────────┘

Edit config.json to enable file source:

{
  "log_manager": {
    "buffer_capacity": 200,
    "thread_pool_size": 4,
    "sink_flush_rate_ms": 500
  },
  "sinks": {
    "console": { "enabled": true },
    "files": [
      { "enabled": true, "path": "/home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log" }
    ]
  },
  "sources": {
    "file": {
      "enabled": true,    ← Change to true
      "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
      "parse_rate_ms": 1900,
      "policy": "cpu"
    },
    "socket": {
      "enabled": false
    },
    "someip": {
      "enabled": false
    }
  }
}


┌────────────────────────────────────────────────────────────┐
│ STEP 3: Run Application                                   │
└────────────────────────────────────────────────────────────┘

# Terminal 1: Run main application
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp


┌────────────────────────────────────────────────────────────┐
│ STEP 4: Expected Output                                   │
└────────────────────────────────────────────────────────────┘

Terminal will show:
┌────────────────────────────────────────────────┐
│ [TelemetryApp] [2025-02-21 14:30:45] [CPU]    │
│ [INFO] [CPU Usage: 45.2%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:47] [CPU]    │
│ [INFO] [CPU Usage: 47.8%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:49] [CPU]    │
│ [INFO] [CPU Usage: 50.1%]                     │
│                                                │
│ ... (continues until end of file)             │
└────────────────────────────────────────────────┘

Note: New line every 1.9 seconds (parse_rate_ms)
      Stops when file ends


┌────────────────────────────────────────────────────────────┐
│ STEP 5: Verify Log Files                                  │
└────────────────────────────────────────────────────────────┘

# Check output log
cat /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# Monitor in real-time
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log


┌────────────────────────────────────────────────────────────┐
│ STOP APPLICATION                                           │
└────────────────────────────────────────────────────────────┘

Press Ctrl+C in terminal

System will:
  • Set isRunning = false
  • Join all threads
  • Flush remaining messages
  • Clean up resources
  • Exit gracefully
```

### Scenario 2: Running with Socket Source

```
┌─────────────────────────────────────────────────────────────┐
│            SOCKET SOURCE CONFIGURATION                      │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ STEP 1: Configure config.json                             │
└────────────────────────────────────────────────────────────┘

Edit config.json to enable socket source:

{
  "sources": {
    "file": {
      "enabled": false
    },
    "socket": {
      "enabled": true,    ← Change to true
      "ip": "127.0.0.1",
      "port": 12345,
      "parse_rate_ms": 1500,
      "policy": "ram"
    },
    "someip": {
      "enabled": false
    }
  }
}


┌────────────────────────────────────────────────────────────┐
│ STEP 2: Start Telemetry Server (Netcat)                   │
└────────────────────────────────────────────────────────────┘

# Terminal 1: Start server
nc -lk 12345

Explanation:
  -l : Listen mode (act as server)
  -k : Keep listening after client disconnects
  12345 : Port number (must match config.json)

You should see:
  (Cursor waiting for input)


┌────────────────────────────────────────────────────────────┐
│ STEP 3: Run Main Application                              │
└────────────────────────────────────────────────────────────┘

# Terminal 2: Run application
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp

You should see:
  (Application connects and waits for data)


┌────────────────────────────────────────────────────────────┐
│ STEP 4: Send Telemetry Data                               │
└────────────────────────────────────────────────────────────┘

# Terminal 1 (netcat): Type telemetry values
67.8
78.5
82.3
91.0
75.2

Press Enter after each value


┌────────────────────────────────────────────────────────────┐
│ STEP 5: Expected Output                                   │
└────────────────────────────────────────────────────────────┘

Terminal 2 will show:
┌────────────────────────────────────────────────┐
│ [TelemetryApp] [2025-02-21 14:30:45] [RAM]    │
│ [INFO] [RAM Usage: 67.8%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:47] [RAM]    │
│ [INFO] [RAM Usage: 78.5%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:49] [RAM]    │
│ [INFO] [RAM Usage: 82.3%]                     │
│                                                │
│ ... (continues as you type)                   │
└────────────────────────────────────────────────┘

Note: Receives data every 1.5 seconds (parse_rate_ms)


┌────────────────────────────────────────────────────────────┐
│ NETWORK DIAGRAM                                            │
└────────────────────────────────────────────────────────────┘

┌──────────────────┐                   ┌──────────────────┐
│  Terminal 1      │                   │  Terminal 2      │
│                  │                   │                  │
│  nc -lk 12345    │                   │  ./ITI_cpp       │
│  (Server)        │                   │  (Client)        │
└────────┬─────────┘                   └────────┬─────────┘
         │                                      │
         │  1. Client connects                  │
         │ ◄────────────────────────────────────┤
         │                                      │
         │  2. Type: "67.8\n"                   │
         ├──────────────────────────────────────►
         │                                      │
         │                         3. Receives "67.8"
         │                            Formats as RAM message
         │                            Logs to sinks
         │                                      │
         │  4. Type: "78.5\n"                   │
         ├──────────────────────────────────────►
         │                                      │
         │                         5. Continues...
         │                                      │


┌────────────────────────────────────────────────────────────┐
│ STOP APPLICATION                                           │
└────────────────────────────────────────────────────────────┘

Terminal 2: Press Ctrl+C (stops client)
Terminal 1: Press Ctrl+C (stops server)
```

### Scenario 3: Running with SOME/IP Source

```
┌─────────────────────────────────────────────────────────────┐
│           SOME/IP SOURCE CONFIGURATION                      │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ STEP 1: Configure config.json                             │
└────────────────────────────────────────────────────────────┘

Edit config.json to enable SOME/IP source:

{
  "sources": {
    "file": {
      "enabled": false
    },
    "socket": {
      "enabled": false
    },
    "someip": {
      "enabled": true,    ← Change to true
      "parse_rate_ms": 1200,
      "policy": "gpu"
    }
  }
}


┌────────────────────────────────────────────────────────────┐
│ STEP 2: Start SOME/IP Service                             │
└────────────────────────────────────────────────────────────┘

# Terminal 1: Run GPU service
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./server

Expected output:
┌────────────────────────────────────────────────┐
│ SOME/IP GPU Service Started                   │
│ Waiting for clients...                        │
│ Service: omnimetron.gpu.GpuUsageData          │
│ Publishing GPU usage data every 1s            │
└────────────────────────────────────────────────┘


┌────────────────────────────────────────────────────────────┐
│ STEP 3: Run Main Application                              │
└────────────────────────────────────────────────────────────┘

# Terminal 2: Run client
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp

Expected output:
┌────────────────────────────────────────────────┐
│ Connecting to SOME/IP service...              │
│ Connected successfully                        │
│ Subscribing to GPU usage events...            │
└────────────────────────────────────────────────┘


┌────────────────────────────────────────────────────────────┐
│ STEP 4: Expected Output                                   │
└────────────────────────────────────────────────────────────┘

Terminal 2 will show:
┌────────────────────────────────────────────────┐
│ [TelemetryApp] [2025-02-21 14:30:45] [GPU]    │
│ [INFO] [GPU Usage: 67.3%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:46] [GPU]    │
│ [INFO] [GPU Usage: 68.1%]                     │
│                                                │
│ [TelemetryApp] [2025-02-21 14:30:48] [GPU]    │
│ [INFO] [GPU Usage: 65.9%]                     │
│                                                │
│ ... (continues indefinitely)                  │
└────────────────────────────────────────────────┘

Note: Data queried every 1.2 seconds (parse_rate_ms)


┌────────────────────────────────────────────────────────────┐
│ SOME/IP COMMUNICATION DIAGRAM                              │
└────────────────────────────────────────────────────────────┘

┌──────────────────┐                   ┌──────────────────┐
│  Terminal 1      │                   │  Terminal 2      │
│                  │                   │                  │
│  ./server        │                   │  ./ITI_cpp       │
│  (Service)       │                   │  (Client)        │
└────────┬─────────┘                   └────────┬─────────┘
         │                                      │
         │  1. Service registers                │
         │     on SOME/IP network               │
         │                                      │
         │  2. Client discovers service         │
         │ ◄────────────────────────────────────┤
         │                                      │
         │  3. Client connects                  │
         │ ◄────────────────────────────────────┤
         │                                      │
         │  4. Client subscribes to events      │
         │ ◄────────────────────────────────────┤
         │                                      │
         │  5. Service publishes GPU data       │
         │     (event notification)             │
         ├──────────────────────────────────────►
         │                                      │
         │  6. Client requests current value    │
         │ ◄────────────────────────────────────┤
         │                                      │
         │  7. Service responds with value      │
         ├──────────────────────────────────────►
         │                                      │
         │  8. Cycle repeats every 1.2s         │
         │                                      │


┌────────────────────────────────────────────────────────────┐
│ STOP APPLICATION                                           │
└────────────────────────────────────────────────────────────┘

Terminal 2: Press Ctrl+C (stops client)
Terminal 1: Press Ctrl+C (stops service)

Order matters:
  1. Stop client first (graceful disconnect)
  2. Then stop service
```

### Scenario 4: Running Multiple Sources Simultaneously

```
┌─────────────────────────────────────────────────────────────┐
│         MULTI-SOURCE CONFIGURATION                          │
└─────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ STEP 1: Configure All Sources                             │
└────────────────────────────────────────────────────────────┘

Edit config.json to enable all sources:

{
  "sources": {
    "file": {
      "enabled": true,    ← All enabled
      "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
      "parse_rate_ms": 1900,
      "policy": "cpu"
    },
    "socket": {
      "enabled": true,    ← All enabled
      "ip": "127.0.0.1",
      "port": 12345,
      "parse_rate_ms": 1500,
      "policy": "ram"
    },
    "someip": {
      "enabled": true,    ← All enabled
      "parse_rate_ms": 1200,
      "policy": "gpu"
    }
  }
}


┌────────────────────────────────────────────────────────────┐
│ STEP 2: Prepare File Data                                 │
└────────────────────────────────────────────────────────────┘

# Create CPU data file
cat > /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt << 'EOF'
45.2
47.8
50.1
52.3
48.9
EOF


┌────────────────────────────────────────────────────────────┐
│ STEP 3: Start All Services (4 Terminals)                  │
└────────────────────────────────────────────────────────────┘

# Terminal 1: SOME/IP Service
./server

# Terminal 2: Socket Server
nc -lk 12345

# Terminal 3: Main Application
./ITI_cpp

# Terminal 4: Monitor Logs
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log


┌────────────────────────────────────────────────────────────┐
│ STEP 4: Send Socket Data                                  │
└────────────────────────────────────────────────────────────┘

# Terminal 2: Type RAM values
67.8
78.5
82.3


┌────────────────────────────────────────────────────────────┐
│ STEP 5: Expected Interleaved Output                       │
└────────────────────────────────────────────────────────────┘

Terminal 3 will show mixed output:
┌────────────────────────────────────────────────┐
│ [TelemetryApp] [14:30:45] [GPU] [INFO]        │
│ [GPU Usage: 67.3%]                            │
│                                                │
│ [TelemetryApp] [14:30:46] [RAM] [INFO]        │
│ [RAM Usage: 67.8%]                            │
│                                                │
│ [TelemetryApp] [14:30:46] [GPU] [INFO]        │
│ [GPU Usage: 68.1%]                            │
│                                                │
│ [TelemetryApp] [14:30:47] [CPU] [INFO]        │
│ [CPU Usage: 45.2%]                            │
│                                                │
│ [TelemetryApp] [14:30:47] [GPU] [INFO]        │
│ [GPU Usage: 65.9%]                            │
│                                                │
│ [TelemetryApp] [14:30:48] [RAM] [INFO]        │
│ [RAM Usage: 78.5%]                            │
│                                                │
│ ... (all sources mixed)                       │
└────────────────────────────────────────────────┘


┌────────────────────────────────────────────────────────────┐
│ MULTI-SOURCE TIMING DIAGRAM                               │
└────────────────────────────────────────────────────────────┘

Time ─────────────────────────────────────────────────────→

0ms
├─ [All threads start]

1200ms
├─ [GPU] First read

1500ms
├─ [RAM] First read

1900ms
├─ [CPU] First read

2400ms
├─ [GPU] Second read

3000ms
├─ [RAM] Second read

3600ms
├─ [GPU] Third read

3800ms
├─ [CPU] Second read

... (continues with different timings)

Result: Logs appear in chronological order regardless of source
```

---

## Configuration Guide

### Complete Configuration Reference

```json
{
  "log_manager": {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // BUFFER CAPACITY
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Number of messages the circular buffer can hold
    // Higher = More buffering, less message loss
    // Lower = Less memory usage, faster overflow
    // 
    // Recommended values:
    //   - Low frequency logging: 50-200
    //   - Normal usage: 200-500
    //   - High frequency: 500-1000
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "buffer_capacity": 200,

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // THREAD POOL SIZE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Number of worker threads for processing writes
    // Higher = More parallel processing
    // Lower = Less resource usage
    // 
    // Recommended values:
    //   - Single core: 1-2
    //   - Dual core: 2-3
    //   - Quad core: 3-4
    //   - 8+ cores: 4-6
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "thread_pool_size": 4,

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // SINK FLUSH RATE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Milliseconds between flushing buffer to sinks
    // Higher = Better throughput, higher latency
    // Lower = Lower latency, more I/O operations
    // 
    // Recommended values:
    //   - Real-time monitoring: 100-300ms
    //   - Normal logging: 500-1000ms
    //   - Batch processing: 1000-5000ms
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "sink_flush_rate_ms": 500
  },

  "sinks": {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CONSOLE SINK
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Writes logs to terminal (stdout)
    // 
    // Use when:
    //   - Development/debugging
    //   - Docker containers (logs to stdout)
    //   - Real-time monitoring needed
    // 
    // Avoid when:
    //   - High-volume production logging
    //   - Running as background service
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "console": {
      "enabled": true
    },

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // FILE SINKS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Writes logs to disk files (persistent storage)
    // 
    // Multiple files for:
    //   - Primary + backup redundancy
    //   - Different log levels (future)
    //   - Separate concerns (errors, audit, etc)
    // 
    // Path requirements:
    //   - Directory must exist
    //   - Write permissions required
    //   - Sufficient disk space
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
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
  },

  "sources": {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // FILE SOURCE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Reads telemetry from local files
    // 
    // Use when:
    //   - Testing with pre-recorded data
    //   - Replaying historical sessions
    //   - Simulating sensor data
    // 
    // File format:
    //   - One value per line
    //   - Example: "45.2\n67.8\n92.1\n"
    // 
    // parse_rate_ms: How often to read next line
    // policy: How to format the data (cpu/ram/gpu)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "file": {
      "enabled": false,
      "path": "/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt",
      "parse_rate_ms": 1900,
      "policy": "cpu"
    },

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // SOCKET SOURCE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Receives telemetry over TCP network
    // 
    // Use when:
    //   - Remote monitoring
    //   - Distributed systems
    //   - Network-connected sensors
    // 
    // Connection:
    //   - TCP client connects to server
    //   - Line-based protocol (newline delimited)
    // 
    // Testing:
    //   - Use: nc -lk 12345
    //   - Type values and press Enter
    // 
    // parse_rate_ms: How often to read from socket
    // policy: How to format the data (cpu/ram/gpu)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "socket": {
      "enabled": false,
      "ip": "127.0.0.1",
      "port": 12345,
      "parse_rate_ms": 1500,
      "policy": "ram"
    },

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // SOME/IP SOURCE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Integrates with automotive middleware
    // 
    // Use when:
    //   - Automotive ECU communication
    //   - Service-oriented architecture
    //   - Event-driven telemetry
    // 
    // Requirements:
    //   - SOME/IP service must be running
    //   - Run: ./server in separate terminal
    // 
    // Communication:
    //   - Request-response: Query current value
    //   - Event-based: Receive notifications
    // 
    // parse_rate_ms: How often to query service
    // policy: How to format the data (cpu/ram/gpu)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    "someip": {
      "enabled": true,
      "parse_rate_ms": 1200,
      "policy": "cpu"
    }
  }
}
```

### Configuration Templates

#### Template 1: Development (Console Only)

```json
{
  "log_manager": {
    "buffer_capacity": 100,
    "thread_pool_size": 2,
    "sink_flush_rate_ms": 300
  },
  "sinks": {
    "console": { "enabled": true },
    "files": []
  },
  "sources": {
    "file": {
      "enabled": true,
      "path": "./test_data.txt",
      "parse_rate_ms": 1000,
      "policy": "cpu"
    },
    "socket": { "enabled": false },
    "someip": { "enabled": false }
  }
}
```

#### Template 2: Production (Files Only)

```json
{
  "log_manager": {
    "buffer_capacity": 500,
    "thread_pool_size": 4,
    "sink_flush_rate_ms": 1000
  },
  "sinks": {
    "console": { "enabled": false },
    "files": [
      { "enabled": true, "path": "/var/log/telemetry/main.log" },
      { "enabled": true, "path": "/var/log/telemetry/backup.log" }
    ]
  },
  "sources": {
    "file": { "enabled": false },
    "socket": { "enabled": false },
    "someip": {
      "enabled": true,
      "parse_rate_ms": 1000,
      "policy": "gpu"
    }
  }
}
```

#### Template 3: Testing (All Sources)

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
      { "enabled": true, "path": "./logs/test.log" }
    ]
  },
  "sources": {
    "file": {
      "enabled": true,
      "path": "./scripts/test_data.txt",
      "parse_rate_ms": 2000,
      "policy": "cpu"
    },
    "socket": {
      "enabled": true,
      "ip": "127.0.0.1",
      "port": 12345,
      "parse_rate_ms": 1500,
      "policy": "ram"
    },
    "someip": {
      "enabled": true,
      "parse_rate_ms": 1000,
      "policy": "gpu"
    }
  }
}
```

---

## Terminal Usage Examples

### Quick Reference Commands

```bash
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# BUILD COMMANDS
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Full rebuild
cd /home/ayman/ITI/Project_cpp_iti/Phases
rm -rf build
mkdir build
cd build
cmake ..
cmake --build . -j$(nproc)

# Incremental build (after code changes)
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
cmake --build .

# Clean build
cmake --build . --target clean
cmake --build .


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# RUN COMMANDS
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Run main application
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./ITI_cpp

# Run with output redirection
./ITI_cpp > app_output.log 2>&1

# Run in background
./ITI_cpp &

# Run SOME/IP service
./server


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# MONITORING COMMANDS
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Watch logs in real-time
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# Check last 50 lines
tail -n 50 /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# Search logs for specific pattern
grep "ERROR" /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# Count log entries
wc -l /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log

# Check file sizes
ls -lh /home/ayman/ITI/Project_cpp_iti/Phases/logs/

# Monitor disk usage
du -sh /home/ayman/ITI/Project_cpp_iti/Phases/logs/


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# PROCESS MANAGEMENT
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Find running process
ps aux | grep ITI_cpp

# Kill process
pkill ITI_cpp
# or
kill $(pgrep ITI_cpp)

# Force kill
kill -9 $(pgrep ITI_cpp)

# Check thread count
ps -eLf | grep ITI_cpp | wc -l


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# DEBUGGING COMMANDS
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Run with GDB
gdb ./ITI_cpp
# Inside GDB:
# (gdb) run
# (gdb) backtrace  (if crash occurs)

# Check for memory leaks with Valgrind
valgrind --leak-check=full ./ITI_cpp

# Monitor system resources
top -p $(pgrep ITI_cpp)

# Check open files
lsof -p $(pgrep ITI_cpp)


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# NETWORK TESTING (Socket Source)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Start TCP server
nc -lk 12345

# Connect to server and send data
echo "67.8" | nc 127.0.0.1 12345

# Send multiple values
for i in {60..80}; do echo "$i.5"; sleep 1; done | nc 127.0.0.1 12345

# Check if port is in use
netstat -tulpn | grep 12345
# or
lsof -i :12345


# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# FILE PREPARATION (File Source)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Create test data
seq 40 60 > /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt

# Add random decimal values
for i in {1..20}; do echo "$((RANDOM % 50 + 40)).$((RANDOM % 10))"; done > test_data.txt

# Verify file content
cat /home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt
```

---

## Complete Usage Scenarios

### Scenario A: Quick Test Run

```bash
┌─────────────────────────────────────────────────────────────┐
│           QUICK TEST (5 MINUTES)                            │
└─────────────────────────────────────────────────────────────┘

# Step 1: Build project (if not already built)
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
cmake --build .

# Step 2: Create test data
echo -e "45.2\n47.8\n50.1" > ../scripts/shell_logs.txt

# Step 3: Edit config (enable file source only)
nano ../config.json
# Set: "file": {"enabled": true}
# Set: "socket": {"enabled": false}
# Set: "someip": {"enabled": false}

# Step 4: Run application
./ITI_cpp

# Step 5: Observe output (Ctrl+C to stop)
# You should see 3 log entries appear

# Step 6: Check log file
cat ../logs/output.log
```

### Scenario B: Full System Test

```bash
┌─────────────────────────────────────────────────────────────┐
│         FULL SYSTEM TEST (ALL SOURCES)                      │
└─────────────────────────────────────────────────────────────┘

# Open 4 terminals

# ════════════════════════════════════════════════════════════
# TERMINAL 1: SOME/IP Service
# ════════════════════════════════════════════════════════════
cd /home/ayman/ITI/Project_cpp_iti/Phases/build
./server


# ════════════════════════════════════════════════════════════
# TERMINAL 2: Socket Server
# ════════════════════════════════════════════════════════════
nc -lk 12345
# Then type values:
# 67.8
# 78.5
# 82.3


# ════════════════════════════════════════════════════════════
# TERMINAL 3: Main Application
# ════════════════════════════════════════════════════════════
cd /home/ayman/ITI/Project_cpp_iti/Phases

# Prepare test file
echo -e "45.2\n47.8\n50.1" > scripts/shell_logs.txt

# Edit config to enable all sources
nano config.json
# Set all "enabled": true

# Run application
cd build
./ITI_cpp


# ════════════════════════════════════════════════════════════
# TERMINAL 4: Monitor Output
# ════════════════════════════════════════════════════════════
tail -f /home/ayman/ITI/Project_cpp_iti/Phases/logs/output.log


# Expected: You'll see interleaved logs from all three sources
# - CPU logs (from file) every 1.9s
# - RAM logs (from socket) every 1.5s
# - GPU logs (from SOME/IP) every 1.2s

# Stop order:
# 1. Ctrl+C in Terminal 3 (main app)
# 2. Ctrl+C in Terminal 2 (socket)
# 3. Ctrl+C in Terminal 1 (SOME/IP)
# 4. Ctrl+C in Terminal 4 (monitor)
```

### Scenario C: Production Deployment

```bash
┌─────────────────────────────────────────────────────────────┐
│           PRODUCTION DEPLOYMENT                             │
└─────────────────────────────────────────────────────────────┘

# Step 1: Build release version
cd /home/ayman/ITI/Project_cpp_iti/Phases
mkdir -p build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# Step 2: Create production directories
sudo mkdir -p /opt/telemetry/bin
sudo mkdir -p /opt/telemetry/config
sudo mkdir -p /var/log/telemetry

# Step 3: Install binaries
sudo cp ITI_cpp /opt/telemetry/bin/
sudo cp server /opt/telemetry/bin/
sudo chmod +x /opt/telemetry/bin/*

# Step 4: Install configuration
sudo cp ../config.json /opt/telemetry/config/

# Edit for production:
sudo nano /opt/telemetry/config/config.json
# - Disable console sink
# - Enable file sinks to /var/log/telemetry/
# - Configure only needed sources

# Step 5: Set permissions
sudo chown -R $USER:$USER /var/log/telemetry
sudo chmod 755 /var/log/telemetry

# Step 6: Create systemd service (optional)
sudo nano /etc/systemd/system/telemetry.service

[Unit]
Description=Telemetry Logging Service
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=/opt/telemetry/bin
ExecStart=/opt/telemetry/bin/ITI_cpp
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target

# Step 7: Enable and start service
sudo systemctl enable telemetry
sudo systemctl start telemetry

# Step 8: Check status
sudo systemctl status telemetry

# Step 9: View logs
sudo journalctl -u telemetry -f
# or
tail -f /var/log/telemetry/output.log

# Step 10: Stop service
sudo systemctl stop telemetry
```

---

## Project Structure

### Complete File Organization

```
/home/ayman/ITI/Project_cpp_iti/Phases/
│
├── CMakeLists.txt                 # Build system configuration
├── config.json                    # Runtime configuration
│
├── app/
│   ├── main.cpp                   # Application entry point
│   └── server.cpp                 # SOME/IP service provider
│
├── Include/
│   ├── LoggingApp.hpp             # Main app orchestrator
│   ├── LogManager.hpp             # Central message hub
│   ├── LogMessage.hpp             # Log entry structure
│   ├── Formatter.hpp              # Policy-based formatters
│   │
│   ├── sinks/
│   │   ├── ILogSink.hpp           # Sink interface
│   │   ├── ConsoleSinkImpl.hpp    # Console output
│   │   └── FileSinkImpl.hpp       # File output
│   │
│   ├── safe/
│   │   ├── SafeFile.hpp           # RAII file wrapper
│   │   └── SafeSocket.hpp         # RAII socket wrapper
│   │
│   └── telemetry/
│       ├── FileTelemetrySourceImpl.hpp      # File source
│       ├── SocketTelemetrySourceImpl.hpp    # Socket source
│       └── SomeIPTelemetrySourceImpl.hpp    # SOME/IP source
│
├── Source/
│   ├── LoggingApp.cpp
│   ├── LogManager.cpp
│   ├── LogMessage.cpp
│   │
│   ├── sinks/
│   │   ├── ConsoleSinkImpl.cpp
│   │   └── FileSinkImpl.cpp
│   │
│   ├── safe/
│   │   ├── SafeFile.cpp
│   │   └── SafeSocket.cpp
│   │
│   └── telemetry/
│       ├── FileTelemetrySourceImpl.cpp
│       ├── SocketTelemetrySourceImpl.cpp
│       └── SomeIPTelemetrySourceImpl.cpp
│
├── gen_src/src-gen/               # Generated SOME/IP code
│   └── v1/omnimetron/gpu/
│       ├── GpuUsageDataProxy.hpp
│       ├── GpuUsageDataSomeIPDeployment.cpp
│       ├── GpuUsageDataSomeIPProxy.cpp
│       └── GpuUsageDataSomeIPStubAdapter.cpp
│
├── build/                         # Build output directory
│   ├── ITI_cpp                    # Main executable
│   ├── server                     # SOME/IP service
│   └── (CMake generated files)
│
├── logs/                          # Runtime log output
│   ├── output.log                 # Primary log file
│   └── backup.log                 # Backup log file
│
└── scripts/                       # Test data files
    └── shell_logs.txt             # Sample telemetry data
```

### Component Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│              Component Dependency Graph                     │
└─────────────────────────────────────────────────────────────┘

main.cpp
    │
    └──→ LoggingApp.hpp
            │
            ├──→ LogManager.hpp
            │       │
            │       ├──→ LogMessage.hpp
            │       │
            │       └──→ ILogSink.hpp
            │               │
            │               ├──→ ConsoleSinkImpl.hpp
            │               │
            │               └──→ FileSinkImpl.hpp
            │                       │
            │                       └──→ SafeFile.hpp
            │
            ├──→ FileTelemetrySourceImpl.hpp
            │       │
            │       └──→ SafeFile.hpp
            │
            ├──→ SocketTelemetrySourceImpl.hpp
            │       │
            │       └──→ SafeSocket.hpp
            │
            ├──→ SomeIPTelemetrySourceImpl.hpp
            │       │
            │       └──→ CommonAPI headers
            │
            └──→ Formatter.hpp
```

---

## Summary

This telemetry logging system provides a robust, production-ready framework for collecting, processing, and storing telemetry data from multiple sources. Key strengths include:

**Flexibility:**
- Multiple input sources (file, socket, SOME/IP)
- Multiple output destinations (console, files)
- Configurable via JSON without recompilation

**Performance:**
- Multi-threaded architecture
- Lock-free circular buffer
- Batched sink writes
- Configurable thread pool

**Reliability:**
- RAII resource management
- Exception-safe design
- Graceful signal handling
- Thread-safe operations

**Usability:**
- Clear configuration structure
- Easy terminal-based testing
- Comprehensive logging
- Well-documented codebase

