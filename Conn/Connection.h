#pragma once
#include "Buffer.h"
#include "../CoLib/ASocket.h"
#include "Buffer.h"
#include <errno.h>
class Connection :NonCopyable
{
public:
    Connection(int fd) :__fd(fd) {}
    ~Connection()
    {
        printf("~connect\n");
    };

    Lazy<ssize_t> recv(Buffer& buf)
    {
        auto iovec = buf.data(false);
        auto n = co_await colib::readv(__fd, iovec.data(), iovec.size());
        if (n < 0)
        {
            co_return n;
        }
        co_return buf.insert(n);
    }

    Lazy<ssize_t> send(Buffer& buf)
    {
        auto iovec = buf.data(true);
        auto n = co_await colib::writev(__fd, iovec.data(), iovec.size());
        if (n < 0)
        {
            co_return n;
        }
        co_return buf.clear(n);
    }

    Lazy<void> close()
    {
        co_await colib::close(__fd);
        __fd = -1;
    }
        
private:
    int __fd;
};