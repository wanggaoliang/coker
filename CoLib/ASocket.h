#pragma once
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "CoRo/Lazy.h"
namespace coasync
{
    Lazy<int> socket(int af, int type, int protocol);

    Lazy<int> listen(int fd, int backlog);

    Lazy<int> accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

    Lazy<ssize_t> readv(int, const struct iovec *, int);

    Lazy<ssize_t> writev(int, const struct iovec *, int);

    Lazy<ssize_t> recv(int sockfd, void *buff, size_t nbytes, int flags);

    Lazy<ssize_t> send(int sockfd, const void *buff, size_t nbytes, int flags);

    Lazy<int> shutdown(int,int);

    Lazy<int> close(int);

};