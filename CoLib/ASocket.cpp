#include "CoKernel.h"
#include "ASocket.h"

Lazy<int> colib::socket(int af, int type, int protocol)
{
    int fd = -1;
    int ret = 0;
    fd = ::socket(af, type | SOCK_NONBLOCK, protocol);
    if (fd > 0)
    {
        ret = co_await CoKernel::getKernel()->updateIRQ(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
        if (ret)
        {
            ::close(fd);
            fd = -1;
        }
    }
    co_return fd;
}

Lazy<int> colib::listen(int fd, int backlog)
{
    int ret = 0;
    ret = ::listen(fd, backlog);
    co_return ret;
}

Lazy<int> colib::accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
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
        return ioret{ clifd,false,false };
    };
    
    auto client = co_await CoKernel::getKernel()->waitFile(fd, EPOLLIN | EPOLLPRI, std::move(acfunc), std::chrono::microseconds(0));
    if (client > 0)
    {
        auto ret = co_await CoKernel::getKernel()->updateIRQ(client, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
        if (ret)
        {
            ::close(client);
            client = -1;
        }
    }
    co_return client;
}

Lazy<int> colib::connect(int fd , const struct sockaddr *addr, int len, std::chrono::microseconds ti )
{
    auto ret = ::connect(fd, addr, len);
    if (ret != 0 && errno == EINPROGRESS)
    {
        auto confunc = [addr, len](int fd, uint events)mutable->ioret {
            int n = 0;
            int connect_error = 0;
            socklen_t errlen = sizeof(connect_error);
            n = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) (&connect_error), &errlen);
            return ioret{ n,false,false };
            };
        ret = co_await CoKernel::getKernel()->waitFile(fd, EPOLLOUT, std::move(confunc), ti);

    }
    co_return ret;
}

Lazy<ssize_t> colib::readv(int fd, const struct iovec *iov, int iovcnt, std::chrono::microseconds ti)
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

Lazy<ssize_t> colib::writev(int fd, const struct iovec *iov, int iovcnt, std::chrono::microseconds ti)
{
    size_t offset = 0;
    int tcnt = 0;
    int sendBytes = 0;
    auto wvfunc = [iov, iovcnt, offset, tcnt, sendBytes](int fd, uint events)mutable->ioret {
        int size = -1;
        std::shared_ptr<iovec[]> tiov(new iovec[iovcnt]);
        if (events & EPOLLRDHUP)
        {
            return ioret{ size,false,false };
        }
        tiov[0].iov_base = iov[tcnt].iov_base + offset;
        tiov[0].iov_len = iov[tcnt].iov_len - offset;
        auto riovcnt = iovcnt - tcnt;
        for (int i = 1;i < riovcnt;i++)
        {
            tiov[i].iov_base = iov[tcnt + i].iov_base;
            tiov[i].iov_len = iov[tcnt + i].iov_len;
        }
        
        while (true)
        {
            size = ::writev(fd, tiov.get(), riovcnt);
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
                else
                {
                    return ioret{ size,false,false };
                }
            }
            break;
        }
        sendBytes += size;
        //计算offset tcnt
        for (int i = 0;i < riovcnt;i++)
        {
            if (tiov[i].iov_len > size)
            {
                offset += size;
                size = 0;
                break;
            }
            else
            {
                offset = 0;
                tcnt++;
                size -= tiov[i].iov_len;
            }
        }
        return ioret{ sendBytes ,tcnt < iovcnt,tcnt < iovcnt };
        };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLOUT, std::move(wvfunc), ti);

}

Lazy<ssize_t> colib::recv(int fd, void *buff, size_t nbytes, int flags, std::chrono::microseconds ti)
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

Lazy<ssize_t> colib::send(int fd, const void *buff, size_t nbytes, int flags, std::chrono::microseconds ti)
{
    int sendbytes = 0;
    auto sendfunc = [buff, nbytes, flags, sendbytes](int fd, uint events)mutable->ioret {
        int size = -1;
        while (true)
        {
            size = ::send(fd, buff + sendbytes, nbytes - sendbytes, flags | MSG_NOSIGNAL);
            if (size < 0 || errno != EINTR)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EWOULDBLOCK)
                {
                    return ioret{ sendbytes, true, true };
                }
                else
                {
                    return ioret{ size, true, false };
                }
            }
            break;
        }
        sendbytes += size;
        return ioret{ sendbytes,sendbytes < nbytes,sendbytes < nbytes };
        };

    co_return co_await CoKernel::getKernel()->waitFile(fd, EPOLLOUT, std::move(sendfunc), ti);

}

Lazy<int> colib::shutdown(int fd, int howto)
{
    co_return 0;
}

Lazy<int> colib::close(int fd)
{
    if (fd < 0)
    {
        co_return -1;
    }
    auto ret = co_await CoKernel::getKernel()->removeIRQ(fd);
    ::close(fd);
    co_return ret;
}