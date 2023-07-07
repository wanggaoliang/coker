#include "CoRo/Lazy.h"
#include "CoRo/Task.h"
#include "CoKernel/CoKernel.h"
#include <iostream>
#include <chrono>
MuCore mLk;

Task test1(int a)
{
    co_await CoKernel::getKernel()->CoRoLock(mLk);
    for (int i = 0;i < 10;i++)
    {
        std::cout <<1<<":"<< a << ":" << i << std::endl;
        auto t = std::chrono::steady_clock::now() + std::chrono::microseconds(1000000);
        co_await CoKernel::getKernel()->waitTime(t);
    }
    co_await CoKernel::getKernel()->CoRoUnlock(mLk);
    co_return;
}

Task test2(int a)
{

    for (int i = 0;i < 10;i++)
    {
        std::cout <<2<<":"<< a << ":" << i << std::endl;
        auto t = std::chrono::steady_clock::now() + std::chrono::microseconds(1000000);
        co_await CoKernel::getKernel()->waitTime(t);
    }
    co_return;
}
Task test3(int a)
{

    for (int i = 0;i < 10;i++)
    {
        std::cout << 3 << ":" << a << ":" << i << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    co_return;
}

Task test()
{
    for (int i = 0;i < 10;i++)
    {
        test3(i);
    }
    co_return;
}
int main()
{
    std::cout << "1" << std::endl;
    CoKernel::INIT(3);
    test();
    std::cout << "3" << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}