#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <iostream>

#define PORT 6666
#define BUFFER_SIZE 1024

int main()
{
    std::vector<int> fv;
    int sockfd, bytes_sent, bytes_received;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char wbuf[BUFFER_SIZE];
    std::string request;

    // 配置地址结构体信息、端口号、IP地址等
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(56567);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd > 0)
    {
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        {
            close(sockfd);
            sockfd = -1;
        }  
    }
    wbuf[0] = 0x05;
    wbuf[1] = 0x01;
    wbuf[2] = 0x00;
    bytes_sent = send(sockfd, wbuf, 3, 0);
    if (bytes_sent == -1)
    {
        goto exit;
    }
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received <2)
    {
        goto exit;
    }
    if (buffer[0] != 0x05 || buffer[1] != 0x00)
    {
        goto exit;
    }
    else
    {
        std::cout << "connect succ" << std::endl;
    }


    wbuf[0] = 0x05;
    wbuf[1] = 0x01;
    wbuf[2] = 0x00;
    wbuf[3] = 0x01;
    wbuf[4] = 0x7F;
    wbuf[5] = 0x00;
    wbuf[6] = 0x00;
    wbuf[7] = 0x01;
    wbuf[8] = 0xDC;
    wbuf[9] = 0xF8;
    bytes_sent = send(sockfd, wbuf, 10, 0);
    if (bytes_sent == -1)
    {
        goto exit;
    }
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 2)
    {
        goto exit;
    }
    if (buffer[0] != 0x05 || buffer[1] != 0x00)
    {
        std::cout << "rep fail" << std::endl;
        goto exit;
    }
    else
    {
        std::cout << "rep succ" << std::endl;
    }

    // 向服务端发送请求、数据
    while (true)
    {
        std::cin >> request;
        bytes_sent = send(sockfd, request.c_str(), request.length(), 0);
        if (bytes_sent == -1)
        {
            goto exit;
        }

        printf("Sent %d bytes to server: %s\n", bytes_sent, request.c_str());
        bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            goto exit;
        }

        printf("Received %d bytes from server: %s\n", bytes_received, buffer);
        bzero(buffer, BUFFER_SIZE);
    }
exit:
    close(sockfd);
    return 0;
}
