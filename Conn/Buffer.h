#pragma once
#include <sys/uio.h>
#include <vector>
#define DEFAULT_BUFFER_SIZE 1024

class Buffer
{
public:
    Buffer(u_int16_t size) : __buf(size), __start(0), __end(0), __size(size)
    {

    };

    Buffer() : Buffer(DEFAULT_BUFFER_SIZE)
    {
    };
    ~Buffer() = default;
    u_int16_t readableCap()const
    {
        return __start <= __end ? __end - __start : __size - __start + __end;
    }
    u_int16_t writeableCap()const
    {
        return __start > __end ? __start - __end - 1 : __size + __start - __end - 1;
    }
    u_int16_t getCap()const
    {
        return __size - 1;
    }
    bool readable()const
    {
        return __start != __end;
    }
    bool writeable()const
    {
        return __start != (__end + 1) % __size;
    }

    char &operator[](int index)
    {
        if (index < 0 || index >= readableCap())
        {
            throw "OutOfRangeException";
        }
        return __buf[(__start + index) % __size];
    }

    int clear()
    {
        int n = readableCap();
        __start = 0;
        __end = 0;
        return n;
    }

    int clear(u_int16_t n)
    {
        if (n > readableCap())
        {
            n = readableCap();
        }
        __start = (__start + n) % __size;
        return n;
    }

    int insert(u_int16_t n)
    {
        if (n > writeableCap())
        {
            n = writeableCap();
        }
        __end = (__end + n) % __size;
        return n;
    }
    int insertCh(char ch)
    {
        if (!writeable())
        {
            return 0;
        }
        __buf[__end]=ch;
        __end = (__end + 1) % __size;
        return 1;
    }

    std::vector<struct iovec> data(bool read)
    {
        std::vector<struct iovec> iov;
        u_int16_t head = 0;
        u_int16_t tail = 0;
        if (read)
        {
            head = __start;
            tail = __end;
        }
        else
        {
            head = __end;
            tail = (__start + __size - 1) % __size;
        }
        if (head < tail)
        {
            iov.emplace_back(__buf.data() + head, tail - head);
        }
        else if (head > tail)
        {
            iov.emplace_back(__buf.data() + head, __size - head);
            if (tail > 0)
            {
                iov.emplace_back(__buf.data(), tail);
            }
        }
        return iov;
    }

    std::string retriveStr(int n)
    {
        std::string str;
        int retrive = 0;
        auto iovs = data(true);
        for (auto iov : iovs)
        {
            auto len = n < iov.iov_len ? n : iov.iov_len;
            str.append(static_cast<char *>(iov.iov_base), len);
            n -= len;
            retrive += len;
        }
        clear(retrive);
        return std::move(str);
    }
    u_int16_t retriveRaw(void*start,int n)
    {
        if (n > readableCap())
        {
            return 0;
        }
        auto p = static_cast<char *>(start);
        for (int i = 0;i < n;i++)
        {
            p[i] = __buf[(__start + i) % __size];
        }
        clear(n);
        return n;
    }
    template<class T>
    u_int16_t retrive(T &in)
    {
        return retriveRaw(&in, sizeof(T));
    }
private:
    std::vector<char> __buf;
    u_int16_t __start;
    u_int16_t __end;
    u_int16_t __size;
};