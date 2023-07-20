#include "CoLib/CoRo/Lazy.h"
#include "Task.h"
#include <type_traits>
#include<functional>

Lazy<int> test(int a)
{
    std::cout << a << std::endl;
    co_return a;
}

int test1(int a)
{
    std::cout << a << std::endl;
    return a;
}


class fiber
{
public:
    template<typename _Callable, typename... _Args>
        requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
    fiber(_Callable &&__f, _Args&&... __args)
    {
        __run(make_tmpLazy(std::forward<_Callable>(__f), std::forward<_Args>(__args)...));

    }
private:
    int id;
    void __run(Lazy<void> &&);
    template<typename _Callable, typename... _Args>
        requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
    static Lazy<void> make_tmpLazy(_Callable &&__f, _Args&&... __args)
    {
        using ret = decltype(std::declval<_Callable>()(__args...));
        //using cla = Template_Type_Traits<ret>::classType;
        if constexpr (HasCoAwaitMethod<ret>)
        {
            co_await __f(__args...);
        }
        else
        {
            __f(__args...);
            co_return;
        }
    }

};

Task makeTask(Lazy<void> &&lazy)
{
    co_await lazy;
}
void fiber::__run(Lazy<void> &&t)
{
    makeTask(std::move(t));
}



int main()
{
    fiber(test1, 2);
    return 0;
}