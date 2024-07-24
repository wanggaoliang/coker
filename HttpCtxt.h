#include "Conn/ConnContext.h"

class HttpCtxt :public ConnContext
{
public:
    HttpCtxt(int a):__a(a%10) {}
    ~HttpCtxt() = default;
    Lazy<void> handle(void *buf, int n) override
    {
        char *buff = (char*)buf;
        buff[n] = '0' + __a;
        buff[n + 1] = '\0';
        co_return;
    }
private:
    int __a;
};
