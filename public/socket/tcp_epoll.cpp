/**
 * @brief 采用select实现tcp服务器
 * 该服务器通过select系统调用实现多路复用，能够同时处理多个客户端连接
 * 监听指定端口，接收客户端连接并回显客户端发送的数据
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>

#define MAX_EVENTS 10

// 初始化TCP服务器，创建监听套接字并绑定到指定端口
int initServer(int port);

int main(int argc, char *argv[])
{
    // 忽略SIGPIPE信号，避免客户端异常关闭导致服务器崩溃
    signal(SIGPIPE, SIG_IGN);

    // 检查命令行参数，确保用户提供了端口号
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: ./tcp_epoll 5005\n");
        return -1;
    }

    // 初始化服务端的监听socket
    int listen_sock = initServer(atoi(argv[1]));
    printf("listen socket: %d\n", listen_sock);
    if (listen_sock < 0)
    {
        printf("initServer() failed.\n");
        return -1;
    }

    // 创建epoll句柄
    int epoll_fd = epoll_create(1);

    // 为listen_sock准备读事件
    struct epoll_event ev;          // 声明事件的数据结构
    ev.data.fd = listen_sock;       // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回
    ev.events = EPOLLIN;            // 指定监听的事件类型

    // 将需要监视的socket及事件加入到epoll_fd中
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) < 0) {
        perror("epoll_ctl failed to add listen socket");
        close(listen_sock);
        close(epoll_fd);
        return -1;
    }

    // 存放epoll返回的事件
    struct epoll_event evs[MAX_EVENTS];

    // 服务器主循环，持续监听客户端连接和数据
    while (true)
    {
        // 等待事件发生
        // 设置超时时间为10秒
        // int in_fds = epoll_wait(epoll_fd, evs, MAX_EVENTS, 10000);
        // 设置无限等待
        int in_fds = epoll_wait(epoll_fd, evs, MAX_EVENTS, -1);

        if (in_fds < 0)
        {
            perror("epoll() failed.");
            return -1;
        }
        else if (in_fds == 0)
        {
            printf("epoll() timeout.\n");
            continue;
        }
        // in_fds > 0表示有事件发生的socket的数量
        else
        {
            // 遍历所有文件描述符，检查哪些有事件发生
            for (int i = 0; i < in_fds; i++)
            {
                if(evs[i].data.fd == listen_sock)
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                    if (client_sock < 0) 
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {  // 忽略非阻塞下的正常无连接情况
                            perror("accept() failed");
                        }
                        continue;  // 跳过无效连接
                    }
                    printf("accept a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // 将客户端套接字设置为非阻塞模式
                    int flags = fcntl(client_sock, F_GETFL, 0);
                    if (fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
                        perror("fcntl() failed for client socket");
                        close(client_sock);
                        continue;
                    }

                    ev.data.fd = client_sock;
                    ev.events = EPOLLIN;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev) < 0) 
                    {
                        perror("epoll_ctl failed to add client socket");
                        close(client_sock);
                        continue;
                    }
                }
                else
                {
                    // 处理客户端数据
                    char buf[1024];
                    memset(buf, 0, sizeof(buf));

                    // 接收客户端数据
                    if (recv(evs[i].data.fd, buf, sizeof(buf), 0) <= 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            continue;
                        // 客户端关闭连接或发生错误
                        printf("client %d disconnected.\n", evs[i].data.fd);

                        // 关闭套接字并从监听集合中移除
                        close(evs[i].data.fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                    }
                    else
                    {
                        // 成功接收到数据，打印并回显给客户端
                        printf("recv from client %d: %s\n", evs[i].data.fd, buf);
                        if (send(evs[i].data.fd, buf, strlen(buf), 0) <= 0)
                        {
                            perror("send() failed.");
                            close(evs[i].data.fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * @brief 初始化服务端的监听端口
 * 创建套接字，设置套接字选项，绑定地址和端口，开始监听
 * @param port 监听的端口号
 * @return 成功返回监听套接字描述符，失败返回-1
 */
int initServer(int port)
{
    // 创建TCP套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket() failed.");
        return -1;
    }

    // 设置套接字选项，允许地址重用
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt() failed");
        close(sock);
        return -1;
    }

    // 配置服务器地址结构
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有可用地址
    server_addr.sin_port = htons(port);              // 转换端口号到网络字节序

    // 绑定套接字到指定地址和端口
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed.");
        close(sock);
        return -1;
    }

    // 开始监听，设置最大连接队列长度为5
    if (listen(sock, 5) < 0)
    {
        perror("listen() failed.");
        close(sock);
        return -1;
    }

    // 设置监听套接字为非阻塞模式
    int flags = fcntl(sock, F_GETFL, 0);
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl() failed.");
        close(sock);
        return -1;
    }

    return sock;
}