#pragma once
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "CoRo/Lazy.h"
#include <chrono>
namespace colib
{
    Lazy<int> socket(int af, int type, int protocol);

    Lazy<int> listen(int, int);

    Lazy<int> accept(int, struct sockaddr *, socklen_t *);

    Lazy<int> connect(int, const struct sockaddr *, int, std::chrono::microseconds ti = std::chrono::microseconds(0));

    Lazy<ssize_t> readv(int, const struct iovec *, int, std::chrono::microseconds ti = std::chrono::microseconds(0));

    Lazy<ssize_t> writev(int, const struct iovec *, int, std::chrono::microseconds ti = std::chrono::microseconds(0));

    Lazy<ssize_t> recv(int, void *, size_t, int, std::chrono::microseconds ti = std::chrono::microseconds(0));

    Lazy<ssize_t> send(int, const void *, size_t, int, std::chrono::microseconds ti = std::chrono::microseconds(0));

    Lazy<int> shutdown(int,int);

    Lazy<int> close(int);

};