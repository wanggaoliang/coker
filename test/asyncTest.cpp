#include "CoLib/CoRo/Lazy.h"
#include "CoLib/AFiber.h"
#include "CoLib/AFuture.h"
#include "CoLib/CoKernel/CoKernel.h"
#include "CoLib/AMutex.h"
#include "Conn/Acceptor.h"
#include "Conn/Connection.h"
#include <string.h>
#include "CoLib/ASocket.h"
#include "Sock5Ctxt.h"

Lazy<int> testconnection(int t)
{
    std::cout << "start...sleep " << t << std::endl;
    co_await colib::sleep_for(std::chrono::seconds(t));
    std::cout << "stop...sleep" << std::endl;
    co_return t;
}

Lazy<void> init(int port)
{
    auto f = colib::async(testconnection, 10);
    std::cout << port << " start...wait" << std::endl;
    auto ret = co_await f.get();
    std::cout << "get cons1 " << ret << std::endl;
    auto f2 = colib::async(testconnection, 3);
    co_await colib::sleep_for(std::chrono::seconds(10));
    std::cout << "stop " << std::endl;
    ret = co_await f2.get();
    std::cout << "get cons2 " << ret << std::endl;
    co_return;
}

int main()
{
    CoKernel::INIT(1, 1);
    std::cout << std::is_same_v<char, colib::lazy_result<int>::type> << std::endl;
    colib::async(init, 56567);
    CoKernel::getKernel()->start();
    return 0;
}