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
    std::string request;
    int n = 0;
    std::cin >> n;
    int port;
    std::cin >> port;
    // 配置地址结构体信息、端口号、IP地址等
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    for (int i = 0;i < n;i++)
    {
        // 创建套接字
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd > 0)
        {
            if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
            {
                close(sockfd);
            }
            else
            {

                printf("Connected to server: %d:%s:%d\n", sockfd, inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
                fv.push_back(sockfd);
            }
        }
    }

    // 向服务端发送请求、数据
    while (true)
    {
        std::cin >> request;
        for (auto fd : fv)
        {
            bytes_sent = send(fd, request.c_str(), request.length(), 0);
            if (bytes_sent == -1)
            {
                perror("send");
                close(fd);
                exit(EXIT_FAILURE);
            }

            printf("Sent %d bytes to server %d: %s\n", bytes_sent, fd, request.c_str());
        }
        for (auto fd : fv)
        {
            bytes_received = recv(fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received == -1)
            {
                perror("recv");
                close(fd);
                exit(EXIT_FAILURE);
            }

            printf("Received %d bytes from server %d: %s\n", bytes_received, fd, buffer);
            bzero(buffer, BUFFER_SIZE);
        }
    }


    // Close the socket
    close(sockfd);

    return 0;
}
