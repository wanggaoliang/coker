/**
 *
 *  Socket.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#include "Socket.h"
#include <assert.h>
#include <netinet/tcp.h>
#include <memory.h>

bool Socket::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in *laddr4 =
            reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 =
            reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port &&
               laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return localaddr.sin6_port == peeraddr.sin6_port &&
               memcmp(&localaddr.sin6_addr,
                      &peeraddr.sin6_addr,
                      sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    assert(sockFd_ > 0);
    int ret;
    if (localaddr.isIpV6())
        ret = ::bind(sockFd_, localaddr.getSockAddr(), sizeof(sockaddr_in6));
    else
        ret = ::bind(sockFd_, localaddr.getSockAddr(), sizeof(sockaddr_in));

    if (ret == 0)
        return;
    else
    {
        exit(1);
    }
}
void Socket::listen()
{
    assert(sockFd_ > 0);
    int ret = ::listen(sockFd_, SOMAXCONN);
    if (ret < 0)
    {
        exit(1);
    }
}
int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    socklen_t size = sizeof(addr6);
    int connfd = ::accept4(sockFd_,
                           (struct sockaddr *)&addr6,
                           &size,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet6(addr6);
    }
    return connfd;
}
void Socket::closeWrite()
{

    if (::shutdown(sockFd_, SHUT_WR) < 0)
    {
    }
}
int Socket::read(char *buffer, uint64_t len)
{
    return recv(sockFd_, buffer, static_cast<int>(len), 0);
}

int Socket::write(char *buffer, uint64_t len)
{
    return send(sockFd_, buffer, static_cast<int>(len), 0);
}

struct sockaddr_in6 Socket::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd,
                      static_cast<struct sockaddr *>((void *)(&localaddr)),
                      &addrlen) < 0)
    {
    }
    return localaddr;
}

struct sockaddr_in6 Socket::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd,
                      static_cast<struct sockaddr *>((void *)(&peeraddr)),
                      &addrlen) < 0)
    {
    }
    return peeraddr;
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_,
                 IPPROTO_TCP,
                 TCP_NODELAY,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

void Socket::setReuseAddr(bool on)
{
#ifdef _WIN32
    char optval = on ? 1 : 0;
#else
    int optval = on ? 1 : 0;
#endif
    ::setsockopt(sockFd_,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT

    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockFd_,
                           SOL_SOCKET,
                           SO_REUSEPORT,
                           &optval,
                           static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
    }
#else
    if (on)
    {
    }
#endif
}

void Socket::setKeepAlive(bool on)
{

    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_,
                 SOL_SOCKET,
                 SO_KEEPALIVE,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

int Socket::getSocketError()
{

    int optval;

    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockFd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

Socket::~Socket()
{
    if (sockFd_ >= 0)
        close(sockFd_);
}
