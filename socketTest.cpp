#include "CoLib/CoRo/Lazy.h"
#include "CoLib/AFiber.h"
#include "CoLib/CoKernel/CoKernel.h"
#include "CoLib/AMutex.h"
#include "Conn/Acceptor.h"
#include "Conn/Connection.h"
#include <string.h>
#include "CoLib/ASocket.h"
#include "Sock5Ctxt.h"

Lazy<void> testconnection(int connfd, int t)
{
    Sock5Ctxt sock5ctxt{ std::make_shared<Connection>(connfd) };
    co_await sock5ctxt.run();
}

Lazy<void> init(int port)
{
    Acceptor ac{};
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    co_await ac.init(std::function([port](int fd, struct sockaddr *addr, socklen_t *addrlen) {
        colib::fiber(testconnection, fd, port / 1111);
                                   }), servaddr);
    co_await ac.run();
    co_await ac.deinit();
}

int main()
{
    CoKernel::INIT(2, 1);
    colib::fiber{ init,56567 };
    std::cout << std::thread::hardware_concurrency() << std::endl;
    CoKernel::getKernel()->start();
    return 0;
}