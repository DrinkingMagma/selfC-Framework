#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

// 设置套接字为非阻塞模式
int setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    int client_sock;
    struct sockaddr_in server_addr;
    char buf[1024];

    // 创建 socket
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    // 设置为非阻塞
    if (setNonBlock(client_sock) < 0)
    {
        close(client_sock);
        return -1;
    }

    // 填充服务器信息
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        printf("Invalid server IP address: %s\n", argv[1]);
        close(client_sock);
        return -1;
    }
    server_addr.sin_port = htons(atoi(argv[2]));

    // 非阻塞 connect
    int ret = connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0 && errno != EINPROGRESS)
    {
        perror("connect failed");
        close(client_sock);
        return -1;
    }

    // 使用 poll 等待连接完成
    struct pollfd pfds;
    pfds.fd = client_sock;
    pfds.events = POLLOUT;

    if (poll(&pfds, 1, 5000) <= 0)  // 最多等5秒
    {
        printf("Connection timeout or poll error.\n");
        close(client_sock);
        return -1;
    }

    // 检查连接是否真正成功
    int so_error = 0;
    socklen_t so_len = sizeof(so_error);
    if (getsockopt(client_sock, SOL_SOCKET, SO_ERROR, &so_error, &so_len) < 0)
    {
        perror("getsockopt error");
        close(client_sock);
        return -1;
    }

    if (so_error != 0)
    {
        printf("Connect failed: %s\n", strerror(so_error));
        close(client_sock);
        return -1;
    }

    printf("Connected to server successfully!\n");

    while (1)
    {
        printf("Please input message (input 'exit' to quit): ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            perror("fgets error");
            break;
        }

        // 去掉换行符
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

        if (strcmp(buf, "exit") == 0)
        {
            printf("Client exit.\n");
            break;
        }

        // 发送数据
        ssize_t total_sent = 0;
        size_t to_send = strlen(buf);

        while (total_sent < to_send)
        {
            ssize_t n = send(client_sock, buf + total_sent, to_send - total_sent, 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    pollfd pfds_send = { client_sock, POLLOUT, 0 };
                    poll(&pfds_send, 1, 5000); // 等待发送缓冲区可写
                    continue;
                }
                perror("send error");
                goto exit;
            }
            total_sent += n;
        }

        // 接收服务器回显
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(client_sock, buf, sizeof(buf) - 1, 0);
        if (n < 0)
        {
            perror("recv error");
            goto exit;
        }
        else if (n == 0)
        {
            printf("Server closed the connection.\n");
            goto exit;
        }

        printf("Server echo: %s\n", buf);
    }

exit:
    close(client_sock);
    return 0;
}
