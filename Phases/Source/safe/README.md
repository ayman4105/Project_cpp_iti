# Safe RAII Wrappers Documentation

## Table of Contents
1. [Overview](#overview)
2. [SafeFile Component](#safefile-component)
3. [SafeSocket Component](#safesocket-component)
4. [RAII Pattern Implementation](#raii-pattern-implementation)
5. [Error Handling Strategy](#error-handling-strategy)
6. [Move Semantics](#move-semantics)
7. [Usage Examples](#usage-examples)

---

## Overview

This module provides **RAII (Resource Acquisition Is Initialization)** wrappers around low-level POSIX file and socket operations. The primary goals are:

- **Automatic Resource Management**: File descriptors and sockets are automatically closed when objects go out of scope
- **Exception Safety**: Resources are properly cleaned up even when exceptions occur
- **Move Semantics**: Efficient transfer of ownership without copying
- **Type Safety**: Encapsulates raw file descriptors in safer C++ classes

**Key Benefits:**
- Prevents resource leaks (file descriptor exhaustion)
- Eliminates manual `close()` calls
- Ensures proper cleanup in all exit paths
- Follows C++ best practices for resource management

---

## SafeFile Component

### Purpose
`SafeFile` is a RAII wrapper around POSIX file operations that automatically manages file descriptor lifecycle. It provides safe, exception-safe file I/O operations.

### Class Structure

```cpp
class SafeFile {
private:
    std::string path;  // File path
    int fd;            // POSIX file descriptor
    
public:
    SafeFile(const std::string &file_path);
    ~SafeFile();
    
    // Move semantics (no copying)
    SafeFile(SafeFile &&other) noexcept;
    SafeFile& operator=(SafeFile &&other) noexcept;
    
    // Deleted copy operations
    SafeFile(const SafeFile&) = delete;
    SafeFile& operator=(const SafeFile&) = delete;
    
    // I/O operations
    void write(const std::string &str);
    bool readLine(std::string &out);
};
```

### Constructor

```cpp
SafeFile::SafeFile(const string &file_path) : path(std::move(file_path))
```

**Operation:**
1. Stores the file path
2. Opens file with `O_RDWR` (read/write mode)
3. Stores file descriptor in `fd`

**Error Handling:**
- If `open()` fails (returns -1):
  - Prints file path to stdout
  - Calls `perror()` to display system error message
  - **Note**: Does not throw exception; sets `fd = -1`

**File Descriptor States:**
- `fd >= 0`: File successfully opened
- `fd == -1`: File open failed (all operations will fail gracefully)

**Flags Used:**
- `O_RDWR`: Open for both reading and writing
- No creation flags: File must already exist

### Write Operation

```cpp
void SafeFile::write(const string &str)
```

**Behavior:**
- Checks if file is open (`fd != -1`)
- Writes string content to file using POSIX `::write()`
- Writes `str.length()` bytes from `str.c_str()`

**Important Notes:**
- No error checking on write operation
- No return value (fire-and-forget)
- Partial writes are not handled
- Does nothing if file is not open

**Improvement Opportunities:**
```cpp
// Current: Silent failure
::write(fd, str.c_str(), str.length());

// Better: Check return value
ssize_t written = ::write(fd, str.c_str(), str.length());
if (written != str.length()) {
    // Handle partial write or error
}
```

### Read Line Operation

```cpp
bool SafeFile::readLine(std::string &out)
```

**Behavior:**
1. Clears output string
2. Validates file descriptor
3. Reads one character at a time until newline (`\n`)
4. Handles EOF and errors

**Return Values:**
- `true`: Line successfully read (or partial data before EOF)
- `false`: Error or EOF with no data

**Character-by-Character Reading:**
```cpp
while (true) {
    ssize_t n = ::read(fd, &ch, 1);
    
    if (n == 0)      // EOF reached
        return !out.empty();
    
    if (n == -1) {   // Error occurred
        if (errno == EINTR) continue;  // Interrupted, retry
        return false;                   // Real error
    }
    
    if (ch == '\n')  // Line delimiter
        return true;
    
    out.push_back(ch);  // Accumulate character
}
```

**Special Cases:**
- **EOF with data**: Returns `true` (partial line)
- **EOF without data**: Returns `false` (nothing read)
- **EINTR**: System call interrupted by signal, automatically retries
- **Newline**: Consumes `\n` but does not include it in output

**Performance Note:**
Reading one byte at a time is inefficient for large files. Consider buffering for production use.

### Move Constructor

```cpp
SafeFile::SafeFile(SafeFile &&other) noexcept
    : path(std::move(other.path)), fd(other.fd)
{
    other.fd = -1;
    other.path.clear();
}
```

**Purpose:** Transfer ownership of file descriptor from one object to another

**Operation:**
1. Move `path` string from `other`
2. Copy file descriptor value
3. Invalidate source object by setting `fd = -1`
4. Clear source path (optional cleanup)

**Why `noexcept`?**
- Move operations should never throw
- Enables certain STL optimizations
- Guarantees exception safety in containers

### Move Assignment Operator

```cpp
SafeFile& SafeFile::operator=(SafeFile &&other) noexcept
{
    if (this != &other) {
        if (fd != -1)
            ::close(fd);  // Close current file
        
        path = std::move(other.path);
        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}
```

**Self-Assignment Protection:**
```cpp
if (this != &other)  // Prevent self-move
```

**Operation:**
1. Check for self-assignment
2. **Close current file** (if any) to prevent leak
3. Steal resources from `other`
4. Invalidate `other`

**Critical Detail:**
Must close existing file descriptor before taking ownership of new one, otherwise the old descriptor leaks.

### Destructor

```cpp
SafeFile::~SafeFile()
{
    if (fd != -1)
        ::close(fd);
}
```

**RAII Core Principle:**
Destructor automatically releases the file descriptor when object goes out of scope.

**Guarantees:**
- Called automatically (stack unwinding)
- Exception-safe (destructors should not throw)
- Prevents resource leaks

### ASCII Diagram: SafeFile Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│                   SafeFile Lifecycle                        │
└─────────────────────────────────────────────────────────────┘

1. CONSTRUCTION
   ┌──────────────────────────────────────┐
   │ SafeFile file("/path/to/data.txt");  │
   └─────────────────┬────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │ fd = open(path, ...)  │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │  fd == -1?            │
         │  Yes: Error (logged)  │
         │  No: Success          │
         └───────────────────────┘

2. USAGE
   ┌──────────────────────────────────────────────┐
   │  file.write("Hello\n");                      │
   │    └─→ ::write(fd, "Hello\n", 6)             │
   │                                              │
   │  std::string line;                           │
   │  file.readLine(line);                        │
   │    └─→ Read char-by-char until '\n'         │
   └──────────────────────────────────────────────┘

3. MOVE (if needed)
   ┌──────────────────────────────────────┐
   │ SafeFile file2 = std::move(file);    │
   └─────────────────┬────────────────────┘
                     │
         ┌───────────▼────────────┐
         │ file2.fd = file.fd     │
         │ file.fd = -1           │
         │ (ownership transferred)│
         └────────────────────────┘

4. DESTRUCTION
   ┌──────────────────────────────────────┐
   │ } // file goes out of scope          │
   └─────────────────┬────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │ ~SafeFile() called    │
         │   if (fd != -1)       │
         │     ::close(fd)       │
         └───────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │ File descriptor freed │
         │ No resource leak!     │
         └───────────────────────┘
```

### SafeFile Data Flow

```
Application Code
      │
      ├──→ write("data")
      │         │
      │         ▼
      │    ┌─────────────────┐
      │    │ SafeFile::write │
      │    └────────┬────────┘
      │             │
      │             ▼
      │    ┌─────────────────┐
      │    │ ::write(fd,...) │
      │    └────────┬────────┘
      │             │
      │             ▼
      │      [File on Disk]
      │
      │
      ├──→ readLine(out)
      │         │
      │         ▼
      │    ┌──────────────────┐
      │    │SafeFile::readLine│
      │    └────────┬─────────┘
      │             │
      │             ▼
      │    ┌──────────────────┐
      │    │ Loop: read 1 byte│
      │    │ until '\n'       │
      │    └────────┬─────────┘
      │             │
      │             ▼
      │    ┌──────────────────┐
      │    │ out = "line data"│
      │    └──────────────────┘
      │
      ▼
  [Automatic cleanup on scope exit]
```

---

## SafeSocket Component

### Purpose
`SafeSocket` is a RAII wrapper around POSIX TCP socket operations. It manages socket lifecycle, handles connection establishment, and provides safe send/receive operations.

### Class Structure

```cpp
class SafeSocket {
private:
    int sockfd;              // Socket file descriptor
    std::string ipAddress;   // Remote IP address
    uint16_t portNumber;     // Remote port
    
public:
    SafeSocket(const std::string &ip, uint16_t port);
    ~SafeSocket();
    
    // Move semantics (no copying)
    SafeSocket(SafeSocket &&other) noexcept;
    SafeSocket& operator=(SafeSocket &&other) noexcept;
    
    // Network operations
    bool sendString(const std::string &message);
    bool receiveLine(std::string &out);
};
```

### Constructor

```cpp
SafeSocket::SafeSocket(const string &ip, uint16_t port)
    : ipAddress(ip), portNumber(port)
```

**Operation Sequence:**

1. **Create Socket**
```cpp
sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
if (sockfd == -1)
    throw std::runtime_error("Failed to create socket: " + strerror(errno));
```
   - `AF_INET`: IPv4 protocol
   - `SOCK_STREAM`: TCP (reliable, connection-oriented)
   - `0`: Default protocol

2. **Configure Address Structure**
```cpp
sockaddr_in addr{};
addr.sin_family = AF_INET;                          // IPv4
addr.sin_port = htons(portNumber);                  // Network byte order
inet_pton(AF_INET, ipAddress.c_str(), &addr.sin_addr);  // IP conversion
```

**Key Functions:**
- `htons()`: Host-to-Network Short (byte order conversion)
- `inet_pton()`: Presentation-to-Network (IP string → binary)

3. **Establish Connection**
```cpp
if (::connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    ::close(sockfd);
    throw std::runtime_error("Failed to connect: " + strerror(errno));
}
```

**Error Handling:**
- **Socket creation fails**: Throws exception immediately
- **Connection fails**: Closes socket *before* throwing exception (prevents leak)

**Exception Safety:**
Constructor either succeeds completely or throws with no resource leak.

### Send Operation

```cpp
bool SafeSocket::sendString(const string &message)
{
    if (sockfd == -1)
        return false;
    
    ssize_t n = ::send(sockfd, message.c_str(), message.size(), 0);
    return n == static_cast<ssize_t>(message.size());
}
```

**Behavior:**
- Validates socket is open
- Sends entire message via `::send()`
- Returns `true` only if *all* bytes were sent

**Return Values:**
- `true`: Complete message sent successfully
- `false`: Socket closed or partial send occurred

**Flags:**
- `0`: No special flags (blocking send)

**Important Limitation:**
Does not handle partial sends. In production, you would loop until all bytes are sent:

```cpp
// Better implementation
size_t total_sent = 0;
while (total_sent < message.size()) {
    ssize_t n = ::send(sockfd, message.c_str() + total_sent, 
                       message.size() - total_sent, 0);
    if (n == -1) return false;
    total_sent += n;
}
return true;
```

### Receive Operation

```cpp
bool SafeSocket::receiveLine(string &out)
```

**Behavior:**
Receives data one byte at a time until newline character is encountered.

**Implementation:**
```cpp
while (true) {
    n = ::recv(sockfd, &c, 1, 0);
    
    if (n == 0)              // Connection closed by peer
        return !out.empty();
    
    if (n == -1) {           // Error
        if (errno == EINTR)  // Interrupted system call
            continue;        // Retry
        return false;        // Real error
    }
    
    if (c == '\n')          // Line delimiter
        break;
    
    out.push_back(c);       // Accumulate character
}
return true;
```

**Special Cases:**
- **Graceful close (n=0)**: Returns `true` if partial data exists
- **EINTR**: Signal interrupted the call, automatically retries
- **Newline**: Delimiter is consumed but not included in output

**Performance Consideration:**
Byte-by-byte reception is slow. Consider buffering:
```cpp
char buffer[4096];
ssize_t n = ::recv(sockfd, buffer, sizeof(buffer), 0);
// Parse buffer for lines
```

### Move Constructor

```cpp
SafeSocket::SafeSocket(SafeSocket &&other) noexcept
    : sockfd(other.sockfd), 
      ipAddress(std::move(other.ipAddress)), 
      portNumber(other.portNumber)
{
    other.sockfd = -1;
}
```

**Operation:**
1. Copy socket descriptor
2. Move IP address string
3. Copy port number
4. Invalidate source socket (`sockfd = -1`)

**Thread Safety:**
Not thread-safe. Do not move socket while another thread is using it.

### Move Assignment Operator

```cpp
SafeSocket& SafeSocket::operator=(SafeSocket &&other) noexcept
{
    if (this != &other) {
        if (sockfd != -1)
            ::close(sockfd);  // Close current connection
        
        sockfd = other.sockfd;
        ipAddress = std::move(other.ipAddress);
        portNumber = other.portNumber;
        other.sockfd = -1;
    }
    return *this;
}
```

**Critical Step:**
Must close existing socket before taking ownership of new one to prevent descriptor leak.

### Destructor

```cpp
SafeSocket::~SafeSocket()
{
    if (sockfd != -1)
        ::close(sockfd);
}
```

**Automatic Cleanup:**
- Closes socket connection
- Releases file descriptor
- Triggered automatically when object goes out of scope

### ASCII Diagram: SafeSocket Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│                  SafeSocket Lifecycle                       │
└─────────────────────────────────────────────────────────────┘

1. CONSTRUCTION
   ┌──────────────────────────────────────────────┐
   │ SafeSocket sock("127.0.0.1", 8080);          │
   └─────────────────────┬────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────────┐
         │ sockfd = socket(AF_INET, ...) │
         └──────────────┬────────────────┘
                        │
                        ▼
         ┌──────────────────────────────┐
         │ Configure sockaddr_in:       │
         │  • IP: 127.0.0.1             │
         │  • Port: 8080                │
         └──────────────┬───────────────┘
                        │
                        ▼
         ┌──────────────────────────────┐
         │ connect(sockfd, addr, ...)   │
         └──────────────┬───────────────┘
                        │
         ┌──────────────▼──────────────┐
         │ Success?                    │
         │  Yes: Ready for I/O         │
         │  No: close() + throw        │
         └─────────────────────────────┘

2. SEND DATA
   ┌──────────────────────────────────────────┐
   │ sock.sendString("GET /\r\n");            │
   └─────────────────┬────────────────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ ::send(sockfd, data, ...) │
         └───────────┬───────────────┘
                     │
                     ▼
              [Network → Server]

3. RECEIVE DATA
   ┌──────────────────────────────────────────┐
   │ std::string response;                    │
   │ sock.receiveLine(response);              │
   └─────────────────┬────────────────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ Loop: recv(sockfd, 1 byte)│
         │ until '\n'                │
         └───────────┬───────────────┘
                     │
                     ▼
         ┌───────────────────────────┐
         │ response = "HTTP/1.1 200" │
         └───────────────────────────┘

4. DESTRUCTION
   ┌──────────────────────────────────────┐
   │ } // sock goes out of scope          │
   └─────────────────┬────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │ ~SafeSocket() called  │
         │   ::close(sockfd)     │
         └───────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │ Connection closed     │
         │ No descriptor leak!   │
         └───────────────────────┘
```

### SafeSocket Connection Flow

```
Client (SafeSocket)                      Server
      │                                     │
      ├──→ socket() → sockfd                │
      │                                     │
      ├──→ connect(sockfd, IP:Port) ───────→│
      │                                     │
      │←─────────── ACK (3-way handshake) ─┤
      │                                     │
   [Connected]                          [Connected]
      │                                     │
      ├──→ send("Hello\n") ────────────────→│
      │                                     │
      │←────────────── recv("Reply\n") ────┤
      │                                     │
      ├──→ close(sockfd) ──────────────────→│
      │                                     │
 [Disconnected]                        [Disconnected]
```

---

## RAII Pattern Implementation

### Core Principles

**RAII (Resource Acquisition Is Initialization)** ensures:
1. **Resource acquisition = Object construction**
2. **Resource release = Object destruction**
3. **Automatic cleanup via C++ scoping rules**

### Benefits in This Implementation

#### 1. Exception Safety

```cpp
void processFile(const std::string &path) {
    SafeFile file(path);
    file.write("data");
    
    // If exception thrown here, destructor still called
    throw std::runtime_error("Something went wrong");
    
    // File automatically closed during stack unwinding
}
```

**Without RAII:**
```cpp
void processFile(const std::string &path) {
    int fd = open(path.c_str(), O_RDWR);
    write(fd, "data", 4);
    
    throw std::runtime_error("Something went wrong");
    
    close(fd);  // NEVER REACHED - RESOURCE LEAK!
}
```

#### 2. Scope-Based Lifetime

```cpp
{
    SafeSocket sock("localhost", 9000);
    sock.sendString("request");
    std::string response;
    sock.receiveLine(response);
    
}  // Socket automatically closed here
```

#### 3. Move Semantics Enable Ownership Transfer

```cpp
SafeFile createFile(const std::string &path) {
    SafeFile file(path);
    file.write("initial data");
    return file;  // Move, not copy
}

SafeFile myFile = createFile("data.txt");
// Ownership transferred, no double-close
```

### Rule of Five Compliance

Both classes implement:
1. ✅ **Destructor**: Releases resource
2. ✅ **Move Constructor**: Transfers ownership
3. ✅ **Move Assignment**: Transfers ownership with cleanup
4. ✅ **Copy Constructor**: Deleted (resource cannot be copied)
5. ✅ **Copy Assignment**: Deleted (resource cannot be copied)

**Why Delete Copy Operations?**
File descriptors and sockets cannot be meaningfully copied. Copying would create two objects pointing to the same descriptor, leading to double-close bugs.

---

## Error Handling Strategy

### SafeFile Error Handling

| Operation | Error Response | Recovery |
|-----------|---------------|----------|
| Constructor | Logs error, sets `fd = -1` | All operations become no-ops |
| write() | Silent failure | No indication of error |
| readLine() | Returns `false` | Caller can check return value |

**Critique:**
- Constructor should throw exception for consistency
- write() should return status or throw

### SafeSocket Error Handling

| Operation | Error Response | Recovery |
|-----------|---------------|----------|
| Constructor (socket creation) | Throws exception | Object not created |
| Constructor (connect) | Closes socket, throws | No resource leak |
| sendString() | Returns `false` | Caller can retry or handle |
| receiveLine() | Returns `false` | Caller can close and retry |

**Better Design:**
Socket constructor uses exceptions, making error handling cleaner at call site.

### Comparison Table

```
┌─────────────────────┬──────────────┬─────────────────┐
│ Aspect              │ SafeFile     │ SafeSocket      │
├─────────────────────┼──────────────┼─────────────────┤
│ Constructor failure │ Sets fd=-1   │ Throws exception│
│ Write failure       │ Silent       │ Returns false   │
│ Read failure        │ Returns false│ Returns false   │
│ Resource cleanup    │ Automatic    │ Automatic       │
└─────────────────────┴──────────────┴─────────────────┘
```

---

## Move Semantics

### Why Move-Only?

File descriptors and sockets are **non-copyable** resources:
- Only one owner should exist
- Copying would create ambiguous ownership
- Closing one copy would invalidate the other

### Move Constructor Pattern

```cpp
SafeFile file1("data.txt");
SafeFile file2 = std::move(file1);

// After move:
// - file2.fd = original descriptor
// - file1.fd = -1 (invalid, safe to destroy)
```

### Move Assignment Pattern

```cpp
SafeFile file1("data.txt");
SafeFile file2("other.txt");

file2 = std::move(file1);  // file2's old descriptor closed first!

// After move:
// - file2.fd = original file1 descriptor
// - file1.fd = -1
// - Original file2 descriptor was closed
```

### Visual: Move Semantics

```
BEFORE MOVE:
┌──────────────┐         ┌──────────────┐
│   file1      │         │   file2      │
├──────────────┤         ├──────────────┤
│ fd = 5       │         │ fd = 7       │
│ path="a.txt" │         │ path="b.txt" │
└──────────────┘         └──────────────┘
       │                        │
       ▼                        ▼
  [descriptor 5]           [descriptor 7]

AFTER: file2 = std::move(file1)
┌──────────────┐         ┌──────────────┐
│   file1      │         │   file2      │
├──────────────┤         ├──────────────┤
│ fd = -1      │         │ fd = 5       │
│ path=""      │         │ path="a.txt" │
└──────────────┘         └──────────────┘
   (invalid)                    │
                                ▼
                           [descriptor 5]
                     
                     [descriptor 7 was closed]
```

---

## Usage Examples

### Example 1: SafeFile Basic Usage

```cpp
#include "SafeFile.hpp"

void logData(const std::string &message) {
    SafeFile logFile("/var/log/app.log");
    
    logFile.write(message);
    logFile.write("\n");
    
    // File automatically closed when logFile goes out of scope
}
```

### Example 2: SafeFile Reading

```cpp
#include "SafeFile.hpp"
#include <iostream>

void processFile(const std::string &path) {
    SafeFile file(path);
    std::string line;
    
    while (file.readLine(line)) {
        std::cout << "Read: " << line << "\n";
        // Process line
    }
    
    // File automatically closed
}
```

### Example 3: SafeSocket HTTP Request

```cpp
#include "SafeSocket.hpp"
#include <iostream>

void fetchWebPage() {
    try {
        SafeSocket sock("example.com", 80);
        
        // Send HTTP request
        sock.sendString("GET / HTTP/1.1\r\n");
        sock.sendString("Host: example.com\r\n");
        sock.sendString("\r\n");
        
        // Read response
        std::string line;
        while (sock.receiveLine(line)) {
            std::cout << line << "\n";
        }
        
        // Socket automatically closed
        
    } catch (const std::exception &e) {
        std::cerr << "Connection failed: " << e.what() << "\n";
    }
}
```

### Example 4: SafeSocket Echo Client

```cpp
#include "SafeSocket.hpp"

void echoClient() {
    SafeSocket client("127.0.0.1", 12345);
    
    std::string message = "Hello Server\n";
    if (!client.sendString(message)) {
        std::cerr << "Send failed\n";
        return;
    }
    
    std::string response;
    if (client.receiveLine(response)) {
        std::cout << "Server replied: " << response << "\n";
    }
    
    // Connection closed automatically
}
```

### Example 5: Move Semantics in Action

```cpp
SafeFile createTempFile() {
    SafeFile temp("/tmp/data.txt");
    temp.write("temporary data\n");
    return temp;  // Moved, not copied
}

void useFile() {
    SafeFile myFile = createTempFile();  // Ownership transferred
    
    std::string line;
    myFile.readLine(line);  // Works perfectly
    
    // File closed when myFile destroyed
}
```

### Example 6: Container Storage

```cpp
#include <vector>

void manageMultipleFiles() {
    std::vector<SafeFile> files;
    
    // Move into vector
    files.push_back(SafeFile("/tmp/file1.txt"));
    files.push_back(SafeFile("/tmp/file2.txt"));
    
    for (auto &file : files) {
        file.write("data\n");
    }
    
    // All files automatically closed when vector destroyed
}
```

---

## Best Practices

### DO:
✅ Use these classes instead of raw file descriptors
✅ Let RAII handle cleanup automatically
✅ Return by value (move semantics handle it)
✅ Store in containers (`std::vector<SafeFile>`)
✅ Check return values from `readLine()` and `sendString()`

### DON'T:
❌ Try to copy these objects
❌ Store raw file descriptors alongside RAII wrappers
❌ Call `close()` manually
❌ Access moved-from objects
❌ Ignore return values

---

## Limitations and Improvements

### Current Limitations

1. **Byte-by-byte I/O**: Very slow for large data
2. **No buffering**: Each read/write is a system call
3. **Limited error reporting**: `write()` has no feedback
4. **No async support**: All operations are blocking
5. **IPv4 only**: No IPv6 support in SafeSocket
6. **No reconnection**: Connection loss requires new object

### Suggested Improvements

#### 1. Add Buffering
```cpp
class BufferedSafeFile : public SafeFile {
    std::array<char, 4096> buffer;
    size_t buffer_pos;
    // ... buffered read/write methods
};
```

#### 2. Better Error Handling
```cpp
class SafeFile {
public:
    struct WriteResult {
        bool success;
        size_t bytes_written;
        int error_code;
    };
    
    WriteResult write(const std::string &data);
};
```

#### 3. Non-blocking I/O
```cpp
SafeSocket(const std::string &ip, uint16_t port, bool non_blocking = false);
```

#### 4. IPv6 Support
```cpp
SafeSocket(const std::string &ip, uint16_t port, int family = AF_INET);
```

---

## Memory and Resource Safety

### Stack Allocation (Recommended)

```cpp
void safeUsage() {
    SafeFile file("data.txt");  // Stack allocated
    // ... use file
}  // Automatically destroyed, resource freed
```

### Heap Allocation (Less Common)

```cpp
void heapUsage() {
    auto file = std::make_unique<SafeFile>("data.txt");
    // ... use file
}  // unique_ptr destructor calls SafeFile destructor
```

### Container Storage

```cpp
std::vector<SafeFile> files;
files.emplace_back("/tmp/file1.txt");  // Constructed in-place
// All files destroyed when vector destroyed
```

---

## Summary

### SafeFile
- RAII wrapper for POSIX file operations
- Automatic file descriptor management
- Move-only semantics
- Line-based reading, string writing
- Safe cleanup even on exceptions

### SafeSocket
- RAII wrapper for TCP sockets
- Automatic connection and cleanup
- Exception-based error handling in constructor
- Line-based text protocol support
- IPv4 TCP client implementation

### Key Takeaways
1. **RAII eliminates resource leaks** by tying resource lifetime to object lifetime
2. **Move semantics** enable efficient ownership transfer
3. **Copy deletion** prevents dangerous double-free bugs
4. **Automatic cleanup** works even with exceptions
5. **Type safety** replaces error-prone raw descriptors

These classes provide a solid foundation for safe, exception-safe I/O operations in C++ applications, particularly suited for embedded systems, telemetry applications, and network clients.