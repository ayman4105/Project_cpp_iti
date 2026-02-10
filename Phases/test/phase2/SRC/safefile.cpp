#include"../INC/safefile.hpp"


safefile::safefile(const string& path) : path{std::move(path)}{
    fd = open(path.c_str() , O_RDWR);
    if(fd == -1){
        perror("error opening file");
    }
}

void safefile::write(const string& str){
    if(fd != -1){
        ::write(fd , str.c_str() , str.length());
    }
}

bool safefile::readline(string& out){
    out.clear();

    if (fd == -1)
        return false;

    char ch;
    while (true) {
        ssize_t n = ::read(fd, &ch, 1);

        if (n == 0) {
            // EOF: reached end of file
            return !out.empty(); // true if we read something, false if nothing
        }

        if (n == -1) {
            if (errno == EINTR)
                continue;   // interrupted, retry
            return false;   // real error
        }

        if (ch == '\n') {
            return true;  // line complete
        }

        out.push_back(ch);  // append char to output
    }

}


safefile::safefile(safefile&& other)noexcept : path{other.path} , fd{other.fd}{
    other.fd = -1;
    other.path.clear();
}


safefile& safefile::operator=(safefile&& other)noexcept{
    if(this != &other){
        if(fd != -1) ::close(fd);

        path = std::move(other.path);
        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}




safefile::~safefile(){
    if(fd != -1) ::close(fd);

}
