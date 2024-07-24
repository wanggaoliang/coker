#include "Conn/ConnContext.h"
#include "Conn/Buffer.h"
#include <iomanip>
#include <string>
#include <arpa/inet.h>
#include <netdb.h>
class EchoCtx :public ConnContext
{
public:
 
    EchoCtx(std::shared_ptr<Connection> conn) :__localConn(conn), __rxBuf(1024),index(sock5Cnt++) {}
    ~EchoCtx()
    {
        std::cout << index << " ~EchoCtx" << std::endl;
    };

    Lazy<void> run() override
    {
        int n = 0;
        while (true)
        {
            std::cout << index << " wait recv" << std::endl;
            n = co_await __localConn->recv(__rxBuf);
            std::cout << index << " recv" << std::endl;
            if (n > 0)
            {
                co_await __localConn->send(__rxBuf);
            }
            else
            {
                break;
            }
        }
        co_await __localConn->close();
    }
private:
    Buffer __rxBuf;
    struct sockaddr_in serv_addr;
    int sockfd;
    std::shared_ptr<Connection> __localConn;
    int index;
    static int sock5Cnt;
};
int EchoCtx::sock5Cnt = 0;