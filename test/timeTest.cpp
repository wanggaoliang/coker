#include "CoRo/Lazy.h"
#include "CoRo/Task.h"
#include "CoKernel/CoKernel.h"
#include "CoKernel/ATime.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string.h>

Task test_thread_sleep(int in)
{
    std::cout << in << ": start to sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(7));
    std::cout << in << ": wake from sleep" << std::endl;
    co_return;
}

Task test_coro_sleep(int in)
{
    std::cout << in << ": start to sleep" << std::endl;
    co_await coasync::sleep_for(std::chrono::seconds(7));
    std::cout << in << ": wake from sleep" << std::endl;
    co_return;
}

Task init()
{
    auto old = std::chrono::steady_clock::now();
    for (int i = 0;i < 5;i++)
    {
        test_coro_sleep(i);
    }
    co_return;
}
int main()
{
    CoKernel::INIT(1);
    init();
    std::cout << "3" << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}