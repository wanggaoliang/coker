#include "CoKernel.h"
#include "ASocket.h"

Lazy<int> coasync::socket(int af, int type, int protocol)
{
    int fd = -1;
    fd = ::socket(af, type | SOCK_NONBLOCK, protocol);
    co_return fd;
}

Lazy<int> coasync::listen(int fd, int backlog)
{
    int ret = 0;
    ret = ::listen(fd, backlog);
    if (!ret)
    {
        ret = co_await CoKernel::getKernel()->updateIRQ(fd, EPOLLIN | EPOLLPRI | EPOLLET);
    }
    co_return ret;
}

Lazy<int> coasync::accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
    auto acfunc = [](int fd, uint events)mutable->ioret {
        int clifd = -1;
        while (true)
        {
            clifd = ::accept4(fd, (struct sockaddr *) NULL, NULL, SOCK_NONBLOCK);
            if (clifd == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return ioret{ clifd,true };
                }
                else if (errno == EINTR)
                {
                    continue;
                }
            }
            break;
        }
        return ioret{ clifd,false };
    };
    
    auto client = co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(acfunc));
    if (client > 0)
    {
        auto ret = co_await CoKernel::getKernel()->updateIRQ(client, EPOLLIN | EPOLLPRI | EPOLLET);
        if (ret)
        {
            ::close(client);
            client = -1;
        }
    }
    co_return client;
}

Lazy<ssize_t> coasync::readv(int fd, const struct iovec *iov, int iovcnt)
{
    auto rvfunc = [iov, iovcnt](int fd, uint events)mutable->ioret {
        int size = -1;
        while (true)
        {
            size = ::readv(fd, iov, iovcnt);
            if (size == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return ioret{ size,true };
                }
                else if (errno == EINTR)
                {
                    continue;
                }
            }
            break;
        }
        return ioret{ size,false };
    };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(rvfunc));
}

Lazy<ssize_t> coasync::writev(int fd, const struct iovec *iov, int iovcnt)
{
    int size = ::writev(fd, iov, iovcnt);
    co_return size;
}

Lazy<ssize_t> coasync::recv(int fd, void *buff, size_t nbytes, int flags)
{
    auto rvfunc = [buff, nbytes, flags](int fd, uint events)mutable->ioret {
        int size = -1;
        while (true)
        {
            size = ::recv(fd, buff, nbytes, flags);
            if (size == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return ioret{ size,true };
                }
                else if (errno == EINTR)
                {
                    continue;
                }
            }
            break;
        }
        return ioret{ size,false };
    };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(rvfunc));
}

Lazy<ssize_t> coasync::send(int fd, const void *buff, size_t nbytes, int flags)
{
    int size = ::send(fd, buff, nbytes, flags);
    co_return size;
}

Lazy<int> coasync::shutdown(int fd, int howto)
{
    co_return 0;
}

Lazy<int> coasync::close(int fd)
{
    if (fd < 0)
    {
        co_return -1;
    }
    auto ret = co_await CoKernel::getKernel()->removeIRQ(fd);
    ::close(fd);
    co_return ret;
}