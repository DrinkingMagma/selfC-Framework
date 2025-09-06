/**
 * @brief 采用select实现tcp服务器
 * 该服务器通过select系统调用实现多路复用，能够同时处理多个客户端连接
 * 监听指定端口，接收客户端连接并回显客户端发送的数据
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <errno.h>

// 初始化TCP服务器，创建监听套接字并绑定到指定端口
int initServer(int port);

// 更新最大文件描述符
void updateMaxFd(pollfd *read_fds, int *max_fd);

int main(int argc, char *argv[])
{
    // 忽略SIGPIPE信号，避免客户端异常关闭导致服务器崩溃
    signal(SIGPIPE, SIG_IGN);

    // 检查命令行参数，确保用户提供了端口号
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: ./tcp_poll 5005\n");
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

    // 存放需要监视的socket
    pollfd fds[2048];
    for(int i = 0; i < 2048; i++)
        fds[i].fd = -1;

    // 监听listen_sock读事件
    fds[listen_sock].fd = listen_sock;
    // 监听读事件(POLLOUT表示写事件)
    fds[listen_sock].events = POLLIN;

    int max_fd = listen_sock;

    // 服务器主循环，持续监听客户端连接和数据
    while (true)
    {
        // 等待事件发生
        // 设置超时时间为10秒
        int in_fds = poll(fds, max_fd + 1, 10000);

        if (in_fds < 0)
        {
            perror("poll() failed.");
            return -1;
        }
        else if (in_fds == 0)
        {
            printf("poll() timeout.\n");
            continue;
        }
        else
        {
            // 遍历所有文件描述符，检查哪些有事件发生
            for (int cur_fd = 0; cur_fd <= max_fd; cur_fd++)
            {
                if (fds[cur_fd].fd < 0)
                    continue;    // 当前文件描述符没有事件，跳过

                if(fds[cur_fd].revents & POLLIN == 0)
                    continue;   // 当前文件描述符没有可读事件，跳过

                // 处理新的客户端连接
                if (cur_fd == listen_sock)
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);

                    // 接受客户端连接(非阻塞模式下可能返回EAGAIN)
                    int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                    if (client_sock < 0)
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                            perror("accept() failed.");
                        continue;
                    }
                    printf("accept() a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // 设置客户端套接字为非阻塞模式
                    int flags = fcntl(client_sock, F_GETFL, 0);
                    if (fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0)
                    {
                        perror("fcntl() failed for client socket");
                        close(client_sock);
                        continue; // 继续处理其他连接
                    }

                    // 将新客户端套接字加入监听集合
                    fds[client_sock].fd = client_sock;
                    fds[client_sock].events = POLLIN;

                    // 更新最大文件描述符
                    if (max_fd < client_sock)
                        max_fd = client_sock;
                }
                else
                {
                    // 处理客户端数据
                    char buf[1024];
                    memset(buf, 0, sizeof(buf));

                    // 接收客户端数据
                    if (recv(cur_fd, buf, sizeof(buf), 0) <= 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            continue;
                        // 客户端关闭连接或发生错误
                        printf("client %d disconnected.\n", cur_fd);

                        // 关闭套接字并从监听集合中移除
                        close(cur_fd);
                        fds[cur_fd].fd = -1;

                        updateMaxFd(fds, &max_fd);
                    }
                    else
                    {
                        // 成功接收到数据，打印并回显给客户端
                        printf("recv from client %d: %s\n", cur_fd, buf);
                        if (send(cur_fd, buf, strlen(buf), 0) <= 0)
                        {
                            perror("send() failed.");
                            close(cur_fd);
                            fds[cur_fd].fd = -1;
                            updateMaxFd(fds, &max_fd);
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
        return -1;
    }

    // 开始监听，设置最大连接队列长度为5
    if (listen(sock, 5) < 0)
    {
        perror("listen() failed.");
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

void updateMaxFd(pollfd *fds, int *max_fd)
{
    // 检查指针有效性
    if (fds == NULL || max_fd == NULL) {
        return;
    }
    // 处理初始max_fd为负数的情况
    if (*max_fd < 0) {
        return;
    }
    // 从当前max_fd开始向下查找有效fd
    for (int i = *max_fd; i >= 0; i--) {
        if (fds[i].fd != -1) {  // 有效fd（-1表示未使用）
            *max_fd = i;
            return;
        }
    }
    *max_fd = -1;  // 无任何有效fd
}