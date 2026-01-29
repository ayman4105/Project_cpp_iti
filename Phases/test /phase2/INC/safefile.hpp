#pragma once

#include<iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <errno.h>

using string = std::string;


class safefile{

    private:
        int fd = -1;
        string path;

    public:

        safefile(const string& path);
        void write(const string& str);
        bool readline(string& out);

        safefile(safefile&& other)noexcept; 
        safefile& operator=(safefile&& other)noexcept;
        
        ~safefile();

};