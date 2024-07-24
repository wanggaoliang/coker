#include "Conn/ConnContext.h"
#include "Conn/Buffer.h"
#include <iomanip>
#include <string>
#include <arpa/inet.h>
#include <netdb.h>
#define SOCK5_VERSION 0x05
class Sock5Ctxt :public ConnContext
{
public:
    typedef enum
    {
        SOCK_INIT,
        SOCK_AUTH,
        SOCK_REQ,
        SOCK_LINKED,
        SOCK_CLOSED,
        SOCK_INVALID,
    }SOCK_STATE;

    class NegoResp
    {
        u_int8_t ver;
        u_int8_t methods;
    };
    class LinkReq
    {
        u_int8_t ver;
        u_int8_t cmd;
        u_int8_t atyp;
        union
        {

        }addr;
        u_int16_t port;
    };
    class LinkResp
    {
        u_int8_t ver;
        u_int8_t rep;
        u_int8_t atyp;
        union
        {

        }addr;
        u_int16_t port;
    };
    Sock5Ctxt(std::shared_ptr<Connection> conn) :__localConn(conn), __state(SOCK_INIT), __rxBuf(1024), __txBuf(1024), index(sock5Cnt++), __linkflag(true){}
    ~Sock5Ctxt()
    {
        std::cout << index << " ~Sock5Ctxt" << std::endl;
    };
    Lazy<ssize_t> handleAuth()
    {
        u_int8_t mNum = 0;
        u_int8_t method = 0xff;
        if (__rxBuf.readableCap() < 3)
        {
            co_return 0;
        }
        if (__rxBuf[0] != SOCK5_VERSION)
        {
            std::cout <<index<< " not sock5" << std::endl;
            co_return 1;
        }
        mNum = __rxBuf[1];
        __rxBuf.clear(2);
        mNum = mNum < 255 ? mNum : 255;
        for (u_int8_t i = 0;i < mNum;i++)
        {
            if (__rxBuf[i] == 0)
            {
                method = 0;
                break;
            }
        }
        __rxBuf.clear(mNum);
        __txBuf.insertCh(SOCK5_VERSION);
        __txBuf.insertCh(method);
        co_await __localConn->send(__txBuf);
        __state = SOCK_REQ;
        co_return 0;
    }

    Lazy<ssize_t> handleReq()
    {
        if (__rxBuf.readableCap() < 5)
        {
            co_return 0;
        }
        if (__rxBuf[3] == 1)
        {
            __rxBuf.clear(4);
            if (__rxBuf.readableCap() < 4)
            {
                co_return 0;
            }

            __rxBuf.retrive(serv_addr.sin_addr.s_addr);
            __rxBuf.retrive(serv_addr.sin_port);
        }
        else if (__rxBuf[3] == 3)
        {
            int len = __rxBuf[4];
            __rxBuf.clear(5);
            if (__rxBuf.readableCap() < len)
            {
                co_return 0;
            }

            auto addr = __rxBuf.retriveStr(len);
            __rxBuf.retrive(serv_addr.sin_port);
            auto hptr = gethostbyname(addr.data());
            if (hptr  == NULL)
            {
                std::cout <<index<< " gethostbyname error for host:" << addr << std::endl;
                co_return 1;
            }
            else
            {

                char   str[32];
                std::cout<<index<<" first address:"<<inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str))<<std::endl;
                serv_addr.sin_addr.s_addr = inet_addr(str);
            }
            
        }
        serv_addr.sin_family = AF_INET;
        sockfd = co_await colib::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd > 0)
        {
            if (co_await colib::connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
            {
                std::cout << index << " connect error " << std::endl;
                co_await colib::close(sockfd);
                co_return 2;
            }
            else
            {
                std::cout << index << " Connected to server:" << sockfd << ":" << inet_ntoa(serv_addr.sin_addr) << ":" << ntohs(serv_addr.sin_port) << std::endl;
            }
        }
        auto cp = (char *) &serv_addr.sin_addr.s_addr;
        __txBuf.insertCh(SOCK5_VERSION);
        __txBuf.insertCh(0x00);
        __txBuf.insertCh(0x00);
        __txBuf.insertCh(0x01);
        __txBuf.insertCh(cp[0]);
        __txBuf.insertCh(cp[1]);
        __txBuf.insertCh(cp[2]);
        __txBuf.insertCh(cp[3]);
        __txBuf.insertCh(0x01);
        __txBuf.insertCh(0xbb);
        co_await __localConn->send(__txBuf);
        __remoteConn = std::make_shared<Connection>(sockfd);
        __rf = colib::async( [this]()->Lazy<void> {
            co_await this->handleRLink();
        } );
        __state = SOCK_LINKED;
        co_return 0;
    }

    Lazy<ssize_t> handleRLink()
    {
        int n = 0;
        while (__linkflag)
        {
            n = co_await  __remoteConn->recv(__txBuf);
            if (n > 0)
            {
                co_await __localConn->send(__txBuf);
            }
            else
            {
                __linkflag = false;
            }
        }
        co_return 0;
    }

    Lazy<void> run() override
    {
        int n = 0;
        while (__linkflag)
        {
            n = co_await __localConn->recv(__rxBuf);
            if (n > 0)
            {
                switch (__state)
                {
                case SOCK_INIT:
                    co_await handleAuth();
                    break;
                case SOCK_REQ:
                    co_await handleReq();
                    break;
                case SOCK_LINKED:
                    co_await __remoteConn->send(__rxBuf);
                    break;
                default:
                    break;
                }
            }
            else
            {
                std::cout << "local err " << n << std::endl;
                __linkflag = false;
            }
        }
        if (__remoteConn)
        {
            std::cout << "to get rlink " << n << std::endl;
            co_await __rf.get();
            std::cout << "got rlink " << n << std::endl;
            co_await __remoteConn->close();
        }
        if (__localConn)
        {
            co_await __localConn->close();
        }   
    }
private:
    SOCK_STATE __state;
    Buffer __rxBuf;
    Buffer __txBuf;
    struct sockaddr_in serv_addr;
    int sockfd;
    std::shared_ptr<Connection> __remoteConn;
    std::shared_ptr<Connection> __localConn;
    std::atomic_bool __linkflag;
    colib::future<void> __rf;
    int index;
    static int sock5Cnt;
};
int Sock5Ctxt::sock5Cnt = 0;