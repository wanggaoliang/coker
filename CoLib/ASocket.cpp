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
        ret = co_await CoKernel::getKernel()->updateIRQ(fd, EPOLLIN | EPOLLRDHUP | EPOLLET);
    }
    co_return ret;
}

Lazy<int> coasync::accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
    auto acfunc = [addr, addrlen](int fd, uint events)mutable->ioret {
        int clifd = -1;
        while (true)
        {
            clifd = ::accept4(fd, (struct sockaddr *) addr, addrlen, SOCK_NONBLOCK);
            if (clifd == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return ioret{ clifd,true,true };
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
    
    auto client = co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(acfunc), std::chrono::microseconds(0));
    if (client > 0)
    {
        auto ret = co_await CoKernel::getKernel()->updateIRQ(client, EPOLLIN | EPOLLRDHUP | EPOLLET);
        if (ret)
        {
            ::close(client);
            client = -1;
        }
    }
    co_return client;
}

Lazy<int> coasync::connect(int fd , const struct sockaddr *addr, int len, std::chrono::microseconds ti )
{
    int ret = ::connect(fd, addr, len);
    if (ret == 0)
    {
        std::cout << "ok imi" << std::endl;
        ret = co_await CoKernel::getKernel()->updateIRQ(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
    }
    else if (ret != 0 && errno == EINPROGRESS)
    {
        std::cout << "ok later" << std::endl;
        auto confunc = [addr, len](int fd, uint events)mutable->ioret {
            int n = 0;
            int connect_error = 0;
            socklen_t errlen = sizeof(connect_error);
            n = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) (&connect_error), &errlen);
            std::cout << "connect err:"<<n<<":"<<connect_error<<":"<<events << std::endl;
            return ioret{ n,true,false };
            };
        ret = co_await CoKernel::getKernel()->updateIRQ(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
        if (!ret)
        {
            ret = co_await CoKernel::getKernel()->waitFile(fd, EPOLLOUT, std::move(confunc), ti);
        }

    }
    co_return ret;
}

Lazy<ssize_t> coasync::readv(int fd, const struct iovec *iov, int iovcnt, std::chrono::microseconds ti)
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
                    return ioret{ size,true,true };
                }
                else if (errno == EINTR)
                {
                    continue;
                }
            }
            break;
        }
        return ioret{ size,false,false };
    };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(rvfunc),ti);
}

Lazy<ssize_t> coasync::writev(int fd, const struct iovec *iov, int iovcnt)
{
    int size = ::writev(fd, iov, iovcnt);
    co_return size;
}

Lazy<ssize_t> coasync::recv(int fd, void *buff, size_t nbytes, int flags, std::chrono::microseconds ti)
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
                    return ioret{ size,true,true };
                }
                else if (errno == EINTR)
                {
                    continue;
                }
            }
            break;
        }
        if (size == nbytes)
        {
            return ioret{ size,false,false };
        }
        else
        {
            return ioret{ size,true,false };
        }
    };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(rvfunc), ti);
}

Lazy<ssize_t> coasync::send(int fd, const void *buff, size_t nbytes, int flags)
{
    int size = ::send(fd, buff, nbytes, flags);
    if (size < 0)
    {
        std::cout << "send err:" << errno << std::endl;
    }
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