#pragma once
#include <functional>
#include "../CoLib/ASocket.h"
#include <errno.h>
using CONNCB = std::function<void(int, struct sockaddr *, socklen_t *)>;
class Acceptor :NonCopyable
{
public:
    Acceptor() :__fd(-1) {}
    ~Acceptor() = default;
    Lazy<int> init(CONNCB connCB, struct sockaddr_in servaddr)
    {
        int ret = 0;
        int one = 1;
        __connCB = connCB;
        if (!__connCB)
        {
            co_return -1;
        }

        __fd = co_await colib::socket(AF_INET, SOCK_STREAM, 0);
        if (__fd < 0)
        {
            co_return -1;
        }
        setsockopt(__fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (bind(__fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        {
            co_return -1;
        }

        if ((ret = co_await colib::listen(__fd, 10)) == -1)
        {
            printf("listen socket error: (errno: %d)\n", errno);
        }
        co_return ret;
    }
    Lazy<void> run()
    {
        while (true)
        {
            int connfd = co_await colib::accept(__fd, NULL, NULL);
            std::cout << "accept:" << connfd << std::endl;
            if (connfd < 0)
            {
                continue;
            }
            __connCB(connfd, NULL, NULL);
        }
    }
    Lazy<void> deinit()
    {
        co_await colib::close(__fd);
        __fd = -1;
    }
private:
    CONNCB __connCB;
    int __fd;
};