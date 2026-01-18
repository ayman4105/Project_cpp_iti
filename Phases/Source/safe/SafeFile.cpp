#include"../../Include/safe/SafeFile.hpp"


SafeFile::SafeFile(const string& file_path) : path(std::move(file_path)) {
    fd = open(path.c_str(), O_RDWR);
    if (fd == -1) {
        std::cout << "file" << path << "\n"; 
        perror("error opening file");
    }
}

void SafeFile::write(const string& str) {
    if (fd != -1) {
        ::write(fd, str.c_str(), str.length());
    }
}

bool SafeFile::readLine(std::string& out)
{
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


// move constructor
SafeFile::SafeFile(SafeFile&& other) noexcept : path(std::move(other.path)), fd(other.fd) {
    other.fd = -1;
    other.path.clear();
}

// move assignment
SafeFile& SafeFile::operator=(SafeFile&& other) noexcept {
    if (this != &other) {
        if (fd != -1) ::close(fd);
        path = std::move(other.path);
        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}

// destructor
SafeFile::~SafeFile() {
    if (fd != -1) ::close(fd);
}