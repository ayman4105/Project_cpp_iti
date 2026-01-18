#pragma once
#pragma once

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <iostream>

class SafeSocket {
public:
    using string = std::string;

private:
    int sockfd = -1;
    string ipAddress;
    uint16_t portNumber;

public:
    SafeSocket(const string& ip, uint16_t port);
    
    bool sendString(const string& message);
    bool receiveLine(string& out); // read until '\n'

    // move semantics
    SafeSocket(SafeSocket&& other) noexcept;
    SafeSocket& operator=(SafeSocket&& other) noexcept;

    // disable copy
    SafeSocket(const SafeSocket&) = delete;
    SafeSocket& operator=(const SafeSocket&) = delete;

    ~SafeSocket();
};
