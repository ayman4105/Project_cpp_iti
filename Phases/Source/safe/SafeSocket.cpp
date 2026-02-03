#include "safe/SafeSocket.hpp"

SafeSocket::SafeSocket(const string &ip, uint16_t port)
    : ipAddress(ip), portNumber(port)
{
    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        throw std::runtime_error("Failed to create socket: " + std::string(std::strerror(errno)));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNumber);
    inet_pton(AF_INET, ipAddress.c_str(), &addr.sin_addr);

    if (::connect(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        ::close(sockfd);
        throw std::runtime_error("Failed to connect: " + std::string(std::strerror(errno)));
    }
}

bool SafeSocket::sendString(const string &message)
{
    if (sockfd == -1)
        return false;
    ssize_t n = ::send(sockfd, message.c_str(), message.size(), 0);
    return n == static_cast<ssize_t>(message.size());
}

bool SafeSocket::receiveLine(string &out)
{
    if (sockfd == -1)
        return false;
    out.clear();
    char c;
    ssize_t n;
    while (true)
    {
        n = ::recv(sockfd, &c, 1, 0);
        if (n == 0)
            return !out.empty(); // connection closed
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            return false; // real error
        }
        if (c == '\n')
            break;
        out.push_back(c);
    }
    return true;
}

// move constructor
SafeSocket::SafeSocket(SafeSocket &&other) noexcept
    : sockfd(other.sockfd), ipAddress(std::move(other.ipAddress)), portNumber(other.portNumber)
{
    other.sockfd = -1;
}

// move assignment
SafeSocket &SafeSocket::operator=(SafeSocket &&other) noexcept
{
    if (this != &other)
    {
        if (sockfd != -1)
            ::close(sockfd);
        sockfd = other.sockfd;
        ipAddress = std::move(other.ipAddress);
        portNumber = other.portNumber;
        other.sockfd = -1;
    }
    return *this;
}

// destructor
SafeSocket::~SafeSocket()
{
    if (sockfd != -1)
        ::close(sockfd);
}