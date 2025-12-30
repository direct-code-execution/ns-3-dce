#ifndef DCE_IOSTREAM_SIMPLE_H
#define DCE_IOSTREAM_SIMPLE_H

// Prevent standard iostream from being included
// This MUST be defined before any code tries to include <iostream>
#ifndef _GLIBCXX_IOSTREAM
#define _GLIBCXX_IOSTREAM 1
#endif

#include <unistd.h>
#include <string.h>
#include <stdio.h>

namespace dce {

/**
 * Output stream for DCE binaries.
 */
class OutputStream {
public:
    explicit OutputStream(int fd) : m_fd(fd) {}

    OutputStream& operator<<(const char* str) {
        if (str) {
            write(m_fd, str, strlen(str));
        }
        return *this;
    }

    OutputStream& operator<<(int val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", val);
        write(m_fd, buf, strlen(buf));
        return *this;
    }

    OutputStream& operator<<(long val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", val);
        write(m_fd, buf, strlen(buf));
        return *this;
    }

    OutputStream& operator<<(unsigned int val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", val);
        write(m_fd, buf, strlen(buf));
        return *this;
    }

    OutputStream& operator<<(unsigned long val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu", val);
        write(m_fd, buf, strlen(buf));
        return *this;
    }

    OutputStream& operator<<(char c) {
        write(m_fd, &c, 1);
        return *this;
    }

    // Manipulator support (for endl, flush)
    OutputStream& operator<<(OutputStream& (*manip)(OutputStream&)) {
        return manip(*this);
    }

    void flush() {
        // No buffering, nothing to flush
    }

private:
    int m_fd;
};

// Global stream objects
static OutputStream cout(STDOUT_FILENO);
static OutputStream cerr(STDERR_FILENO);
static OutputStream clog(STDERR_FILENO);

// Manipulators
inline OutputStream& endl(OutputStream& os) {
    os << '\n';
    os.flush();
    return os;
}

inline OutputStream& flush(OutputStream& os) {
    os.flush();
    return os;
}

}

// Override std::cout with dce::cout
namespace std {
    // Import dce::OutputStream into std namespace
    using ::dce::OutputStream;

    // Create references to dce streams in std namespace
    static ::dce::OutputStream& cout = ::dce::cout;
    static ::dce::OutputStream& cerr = ::dce::cerr;
    static ::dce::OutputStream& clog = ::dce::clog;

    // Also provide endl/flush in std namespace
    inline ::dce::OutputStream& endl(::dce::OutputStream& os) {
        return ::dce::endl(os);
    }
    inline ::dce::OutputStream& flush(::dce::OutputStream& os) {
        return ::dce::flush(os);
    }
}

#endif // DCE_IOSTREAM_SIMPLE_H
