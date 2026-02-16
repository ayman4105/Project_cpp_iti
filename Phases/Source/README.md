
# Telemetry Logging System Documentation

## 1. `LogMessage` Class

**Purpose:**
`LogMessage` represents a single log entry in the system. Each log contains information about the source, context, severity, timestamp, and the actual message.

**Key Attributes:**

* `app_name` – Name of the application or module generating the log.
* `context` – Specific context or component name (e.g., CPU, Socket).
* `message` – The textual message describing the event.
* `level` – Severity level of the message (Info, Warning, Critical, etc.).
* `time` – Timestamp when the log was created.

**Constructor:**

```cpp
LogMessage(const std::string &app, const std::string &cntxt, const std::string &msg, severity_level sev, std::string time);
```

* Initializes a log with all necessary fields.

**Operator Overload for `<<`:**

```cpp
std::ostream &operator<<(std::ostream &os, const LogMessage &msg);
```

* Formats the log message for output to console or file:

```
[APP_NAME] [TIMESTAMP] [CONTEXT] [LEVEL] [MESSAGE]
```

**Example:**

```cpp
LogMessage msg("CPU", "CPU", "Normal: 45%", severity_level::Info, "2026-02-16 20:00:00");
std::cout << msg;
```

**Output:**

```
[CPU] [2026-02-16 20:00:00] [CPU] [Info] [Normal: 45%]
```

---

## 2. `LogManager` Class

**Purpose:**
`LogManager` manages the storage and dispatching of log messages to multiple sinks (like console, files). It also handles concurrency using a thread pool.

**Key Responsibilities:**

* Maintain a buffer (`messages`) to temporarily hold log messages.
* Accept logs via `log()` or `operator<<`.
* Push write tasks to the thread pool to flush logs to sinks.
* Manage multiple sinks via `add_sink()`.

**Functions:**

### `add_sink`

```cpp
void add_sink(std::unique_ptr<ILogSink> sink);
```

* Adds a new sink (console, file, etc.) to the list of sinks.
* Uses `std::unique_ptr` to manage ownership automatically.

### `log`

```cpp
void log(const LogMessage &message);
```

* Attempts to push the message into the buffer.
* If buffer is full, the message is dropped and a warning is printed:

```
[LogManager] buffer full, message dropped
```

* Then schedules a write task via the thread pool.

### `write`

```cpp
void write();
```

* Pops messages from the buffer one by one.
* Sends each message to all registered sinks by calling their `write()` method.

### `operator<<`

```cpp
LogManager &operator<<(const LogMessage &message);
```

* Convenient syntax for logging:

```cpp
logger << msg;
```

**ASCII Flow Diagram of `LogManager`:**

```
   +-----------------+
   |   LogManager    |
   +-----------------+
        | log()
        v
   +-----------------+
   |   MessageBuffer |
   +-----------------+
        | trypop()
        v
   +-----------------+
   |     Sinks       |
   | (Console/File)  |
   +-----------------+
```

---

## 3. `TelemetryLoggingApp` Class

**Purpose:**
`TelemetryLoggingApp` orchestrates telemetry collection from different sources (file, socket, SOMEIP) and forwards the logs to `LogManager`. It manages concurrency, configuration, and clean shutdown.

**Key Responsibilities:**

* Load configuration from JSON file.
* Initialize sinks (console/file).
* Start telemetry sources in separate threads.
* Start writer thread to flush logs periodically.
* Handle graceful shutdown via `Ctrl+C`.

**Key Attributes:**

* `isRunning` – atomic flag to control thread loops.
* `logger` – instance of `LogManager`.
* `sinks` – list of sink objects.
* `sourceThreads` – list of threads for telemetry sources.
* `writerThread_` – thread to flush logs.

**Core Functions:**

### `loadConfig(path)`

* Reads JSON config file and sets parameters:

  * `buffer_capacity`
  * `thread_pool_size`
  * `sink_flush_rate_ms`
* Enables/disables sources and sinks.

### `setupSinks()`

* Adds console and file sinks according to config.

### `setupTelemetrySources()`

* Starts threads for each enabled source.
* Each thread reads telemetry, formats into `LogMessage`, and logs it.

### `startWriterThread()`

* Periodically flushes messages from `LogManager` to sinks.
* Runs until `isRunning` becomes false.

### `start()`

* Sets `isRunning = true`.
* Starts writer thread.
* Starts telemetry source threads.
* Joins all threads, blocking `main` until shutdown.

---

**ASCII Flow Diagram of `TelemetryLoggingApp`:**

```
          +-------------------------+
          | TelemetryLoggingApp     |
          +-------------------------+
          |  isRunning              |
          |  logger (LogManager)    |
          |  sinks                  |
          +-------------------------+
                   |
                   v
        +-------------------+
        |  Writer Thread    |
        +-------------------+
                 |
                 v
          +---------------+
          |  LogManager   |
          +---------------+
                 |
                 v
        +-----------------+
        |    Sinks        |
        | Console / File  |
        +-----------------+

Telemetry Sources Threads (File / Socket / SOMEIP):
        +-----------------+
        | File Source     |
        +-----------------+
        | Socket Source   |
        +-----------------+
        | SOMEIP Source   |
        +-----------------+
        Each formats -> LogMessage -> logger->log()
```

---

**Summary Flow:**

1. `TelemetryLoggingApp::start()`
   → Starts writer + sources threads.
2. Sources read data → format into `LogMessage`.
3. `LogManager::log()` → pushes message into buffer and schedules write.
4. Writer thread periodically calls `LogManager::write()` → sends to sinks.
5. On `Ctrl+C` → `isRunning = false` → threads finish → program exits.

---
تحب أعملها دلوقتي؟
