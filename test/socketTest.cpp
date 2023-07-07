#include "CoRo/Lazy.h"
#include "CoRo/Task.h"
#include "CoKernel/CoKernel.h"
#include "CoKernel/ASocket.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string.h>

Task testconnection(int connfd)
{
    char    buff[4096];
    int     n;
    while (true)
    {
        n = co_await coasync::recv(connfd, buff, 128, 0);
        if (n > 0)
        {
            buff[n] = '\0';
            printf("recv msg from client %p %d: %s\n", buff, n, buff);
            co_await coasync::send(connfd, buff, 128, 0);
        }
        else
        {
            std::cout << "receive err:" << n << std::endl;
            break;
        }
    }

    co_await coasync::close(connfd);
}

Task testwwwrrr()
{
    int    listenfd, connfd;
    struct sockaddr_in     servaddr;
    int ret = 0;
    int one = 1;

    if ((listenfd = co_await coasync::socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        exit(0);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
    {
        exit(0);
    }

    if ((ret = co_await coasync::listen(listenfd, 10)) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    while (true){
        connfd = co_await coasync::accept(listenfd, NULL, NULL);
        std::cout << "accept:" << connfd << std::endl;
        if (connfd < 0)
        {
            continue;
        }
        testconnection(connfd);
    }
    co_await coasync::close(listenfd);
}
int main()
{
    CoKernel::INIT(4);
    testwwwrrr();
    std::cout << "3" << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}