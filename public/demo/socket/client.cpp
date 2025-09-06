#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        printf("Example: ./client 192.168.59.132 5005\n");
        return -1;
    }

    int client_sock = -1;  // 初始化为无效值，避免关闭未初始化的描述符
    struct sockaddr_in server_addr;
    char buf[1024];
    ssize_t len;  // 用ssize_t处理recv/send的返回值（可能为负）

    // 创建socket
    if((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Create client socket error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    // 初始化服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)  // 替代inet_addr，支持IPv6扩展
    {
        printf("Invalid server IP: %s\n", argv[1]);
        close(client_sock);
        return -1;
    }
    server_addr.sin_port = htons(atoi(argv[2]));

    // 连接服务器
    if(connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connect(%s:%s) error: %s(errno: %d)\n", argv[1], argv[2], strerror(errno), errno);
        close(client_sock);  // 连接失败时关闭socket
        return -1;
    }
    printf("Connect to server(%s:%s) successfully!\n", argv[1], argv[2]);

    // 循环：直到用户输入exit或出错
    while(1)
    {
        printf("Please input message (input 'exit' to quit): ");
        fflush(stdout);  // 确保提示语及时输出

        // 读取用户输入（支持空格，限制长度）
        if(fgets(buf, sizeof(buf), stdin) == NULL)
        {
            printf("Input error: %s(errno: %d)\n", strerror(errno), errno);
            break;
        }
        // 去除fgets读取的换行符
        len = strlen(buf);
        if(len > 0 && buf[len-1] == '\n')
            buf[len-1] = '\0';

        // 处理空输入（仅回车或空格）
        if(strlen(buf) == 0)
        {
            printf("Input nothing, please input again!\n");
            continue;
        }

        // 退出逻辑
        if(strcmp(buf, "exit") == 0)
        {
            printf("Client exit.\n");
            break;
        }

        // 发送数据（确保完整发送）
        len = strlen(buf);
        ssize_t sent = 0;
        while(sent < len)
        {
            ssize_t n = send(client_sock, buf + sent, len - sent, 0);
            if(n < 0)
            {
                printf("Send message error: %s(errno: %d)\n", strerror(errno), errno);
                goto exit;  // 出错时跳转到资源清理
            }
            sent += n;
        }

        // 接收回显（处理服务器关闭的情况）
        memset(buf, 0, sizeof(buf));
        len = recv(client_sock, buf, sizeof(buf)-1, 0);  // 留1字节存\0
        if(len < 0)
        {
            printf("Receive message error: %s(errno: %d)\n", strerror(errno), errno);
            goto exit;
        }
        else if(len == 0)
        {
            printf("Server has closed the connection.\n");
            goto exit;
        }
        printf("Server echo: %s\n", buf);
    }

exit:  // 统一的资源清理出口
    if(client_sock != -1)
        close(client_sock);
    return 0;
}