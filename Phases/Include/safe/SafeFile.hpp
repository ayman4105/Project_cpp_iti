#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <errno.h>

class SafeFile
{
public:
    using string = std::string;

private:
    string path;
    int fd = -1;

public:
    SafeFile(const string &file_path);
    void write(const string &str);
    bool readLine(string &out);

    // move
    SafeFile(SafeFile &&other) noexcept;
    SafeFile &operator=(SafeFile &&other) noexcept;

    // copy
    SafeFile(const SafeFile &) = delete;
    SafeFile &operator=(const SafeFile &) = delete;

    ~SafeFile();
};
