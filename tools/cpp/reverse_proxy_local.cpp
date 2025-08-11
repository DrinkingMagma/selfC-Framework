/**
 * @brief 反向网络代理服务器-内网端
*/
#include "_public.h"
using namespace idc;

void _help();

int connectToDest(const char *ip, const int port, bool is_block=false);

int cmd_connect_socket; // 与外网代理程序的命令通道的socket
int epoll_fd = 0;       // epoll的句柄
int timer_fd = 0;       // 定时器的句柄

#define MAX_SOCKET_NUM 1024        // 最大连接数
#define MAX_EVS 100
int client_sockets[MAX_SOCKET_NUM];    // 保存所有连接的socket
int client_a_time[MAX_SOCKET_NUM];     // 保存所有连接的socket的最后活跃时间
string client_buffer[MAX_SOCKET_NUM];  // 保存所有连接的发送内容的缓存

clogfile log_file;   // 日志文件
cpactive pactive;   // 进程心跳

void EXIT(int sig);
int setNonBlock(int fd);

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        _help();
        return -1;
    }

    close_io_and_signal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if(log_file.open(argv[1]) == false)
    {
        perror("log file open failed.\n");
        return -1;
    }

    memset(client_sockets, -1, sizeof(client_sockets));
    memset(client_a_time, 0, sizeof(client_a_time));
    pactive.add_p_info(30, "reverse_proxy_local");

    cmd_connect_socket = connectToDest(argv[2], atoi(argv[3]), true);
    if(cmd_connect_socket < 0)
    {
        log_file.write("connect %s:%s failed.\n", argv[2], argv[3]);
        return -1;
    }
    client_a_time[cmd_connect_socket] = time(NULL);
    log_file.write("---------------------new-----------------------------\n");
    log_file.write("create wan link success(cmd_connnect_socket = %d)\n", cmd_connect_socket);

    // 命令通道建立后，再设置为非阻塞模式
    setNonBlock(cmd_connect_socket);

    // 创建epoll实例
    epoll_fd = epoll_create1(0);
    // 声明事件的数据结构
    struct epoll_event ev;

    // 监听cmd_connect_socket的可读事件
    ev.events = EPOLLIN;
    ev.data.fd = cmd_connect_socket;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cmd_connect_socket, &ev);

    // --------------------------------------------------------------------
    // 将定时器加入epoll中
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); // 创建定时器文件描述符
    struct itimerspec time_out;                     // 定时器结构体
    memset(&time_out, 0, sizeof(time_out));
    time_out.it_value.tv_sec = 20;                  // 设置定时器间隔(秒)
    time_out.it_value.tv_nsec = 0;
    timerfd_settime(timer_fd, 0, &time_out, NULL);  // 设置定时器，开始计时
    ev.data.fd = timer_fd;                          // 为定时器准备事件
    ev.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev);  // 将定时器文件描述符加入epoll中
    // --------------------------------------------------------------------
    
    // --------------------------------------------------------------------
    // 将信号加入epoll中
    sigset_t sig_set;                                   // 定义信号集
    sigemptyset(&sig_set);                              // 清空信号集
    sigaddset(&sig_set, SIGINT);                        // 添加信号SIGINT
    sigaddset(&sig_set, SIGTERM);                       // 添加信号SIGTERM
    sigprocmask(SIG_BLOCK, &sig_set, NULL);             // 将信号集加入进程屏蔽字（当前进程无法接收信号集中的信号）
    int sig_fd = signalfd(-1, &sig_set, 0);             // 创建信号文件描述符
    ev.data.fd = sig_fd;
    ev.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sig_fd, &ev);    // 将信号文件描述符加入epoll监听
    // --------------------------------------------------------------------

    // 存放epoll返回的事件
    struct epoll_event evs[MAX_EVS];
    
    while (true)
    {
        int in_fds = epoll_wait(epoll_fd, evs, MAX_EVS, -1);
        if(in_fds < 0)
        {
            log_file.write("epoll_wait error\n");
            EXIT(-1);
        }

        // 遍历所有事件
        for(int i = 0; i < in_fds; ++i)
        {
            if(evs[i].data.fd != timer_fd)
                log_file.write("current fd: %d, event: %d\n", evs[i].data.fd, evs[i].events);

            // 若定时器触发，则：1)更新定时器；2)更新进程心跳；3)清理空闲的进程
            if(evs[i].data.fd == timer_fd)
            {
                // log_file.write("the timer is triggered\n");

                timerfd_settime(timer_fd, 0, &time_out, NULL);      // 重新计时

                pactive.upt_atime();

                // 清理空闲的客户端socket
                for(int j = 0; j < MAX_SOCKET_NUM; j++)
                {
                    if(client_sockets[j] > 0 && (time(NULL) - client_a_time[j]) > 90)
                    {
                        log_file.write("close the timeout socket (%d - %d)\n", client_sockets[j], client_sockets[client_sockets[j]]);
                        close(client_sockets[client_sockets[j]]);
                        close(client_sockets[j]);
                        client_sockets[client_sockets[j]] = -1;
                        client_sockets[j] = -1;
                    }
                }
                continue;
            }

            // 若监听到信号
            if(evs[i].data.fd == sig_fd)
            {
                struct signalfd_siginfo sig_info;
                if(read(sig_fd, &sig_info, sizeof(sig_info)) < 0)
                {
                    log_file.write("read signal error.");
                    continue;
                }

                if(sig_info.ssi_signo == SIGTERM)
                {
                    log_file.write("recv signal: SIGTERM.\n");
                    EXIT(SIGTERM);
                }
                else
                if(sig_info.ssi_signo == SIGINT)
                {
                    log_file.write("recv signal: SIGINT.\n");
                    EXIT(SIGINT);
                }
            }
            
            // 若命令通道发生事件
            if(evs[i].data.fd == cmd_connect_socket)
            {
                // 通过命令通道向内网发送命令，将路由参数传递过去
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                if(recv(evs[i].data.fd, buffer, sizeof(buffer), 0) <= 0)
                {
                    log_file.write("recv info from wan error.\n");
                    EXIT(-1);
                }

                // 忽略心跳包
                if(strcmp(buffer, "<active_test>") == 0)
                    continue;

                // 向外网服务端发起新的连接
                int src_socket = connectToDest(argv[2], atoi(argv[3]));
                if(src_socket < 0)
                {
                    continue;
                }
                if(src_socket >= MAX_SOCKET_NUM)
                {
                    log_file.write("when connect wan program socket, too many sockets(%d > %d)\n", src_socket, MAX_SOCKET_NUM);
                    close(src_socket);
                    continue;
                }

                // 从报文中获取目标服务器的地址和端口
                char dst_ip[11];
                int dst_port;
                get_xml_buffer(buffer, "dst_ip", dst_ip, 30);
                get_xml_buffer(buffer, "dst_port", dst_port);

                // 向目标服务器的地址和端口发起socket连接
                int dst_socket = connectToDest(dst_ip, dst_port);
                if(dst_socket < 0)
                {
                    log_file.write("connect to %s:%d error\n", dst_ip, dst_port);
                    close(src_socket);
                    close(dst_socket);
                    continue;
                }
                if(dst_socket >= MAX_SOCKET_NUM)
                {
                    log_file.write("when connect wan program socket, too many sockets(%d > %d)\n", dst_socket, MAX_SOCKET_NUM);
                    close(src_socket);
                    close(dst_socket);
                    continue;
                }
                log_file.write("connect to %s:%d success(%d <--> %d)\n", dst_ip, dst_port, src_socket, dst_socket);

                // 为两个socket准备读事件并添加到epoll中
                ev.data.fd = src_socket;
                ev.events = EPOLLIN;
                setNonBlock(src_socket);
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, src_socket, &ev) < 0)
                {
                    log_file.write("epoll_ctl add source socket error: %s\n", strerror(errno));
                    close(src_socket);
                    close(dst_socket);
                    continue;;
                }
                ev.data.fd = dst_socket;
                ev.events = EPOLLIN;
                setNonBlock(dst_socket);
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dst_socket, &ev) < 0)
                {
                    log_file.write("epoll_ctl add destination socket error: %s\n", strerror(errno));
                    close(src_socket);
                    close(dst_socket);
                    break;
                }

                // 更新client_sockets数组中的值以及最后活跃时间
                client_sockets[src_socket] = dst_socket;
                client_sockets[dst_socket] = src_socket;
                client_a_time[src_socket] = time(0);
                client_a_time[dst_socket] = time(0);

                continue;
            }

            // 处理客户端连接的socket的事件
            // 若为读事件，则需将数据存放在对端socket的缓冲区中
            if(evs[i].events & EPOLLIN)
            {
                char buffer[4096];
                int buffer_len;

                // 读取数据
                buffer_len = read(evs[i].data.fd, buffer, sizeof(buffer));
                // 若读取失败，则关闭socket
                if(buffer_len <= 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                            continue;
                    log_file.write("read error, close client socket(%d - %d)\n", evs[i].data.fd, client_sockets[evs[i].data.fd]);
                    close(evs[i].data.fd);
                    close(client_sockets[evs[i].data.fd]);
                    client_sockets[client_sockets[evs[i].data.fd]] = -1;
                    client_sockets[evs[i].data.fd] = -1;
                    continue;
                }
                log_file.write("recv %d bytes from %d\n", buffer_len, evs[i].data.fd);

                // 将读取的数据转发给对端
                client_buffer[client_sockets[evs[i].data.fd]].append(buffer, buffer_len);

                // 为对端socket添加写事件
                ev.data.fd = client_sockets[evs[i].data.fd];
                ev.events = EPOLLIN | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev);

                //更新两端socket的最后活跃时间
                client_a_time[evs[i].data.fd] = time(0);
                client_a_time[client_sockets[evs[i].data.fd]] = time(0);
            }
            // 若为写事件
            if(evs[i].events & EPOLLOUT)
            {
                int write_len = send(evs[i].data.fd, client_buffer[evs[i].data.fd].data(), client_buffer[evs[i].data.fd].length(), 0);

                log_file.write("send %d bytes to %d\n", write_len, evs[i].data.fd);

                // 在缓冲区中清除已成功发送的数据
                client_buffer[evs[i].data.fd].erase(0, write_len);

                // 若缓冲区为空，则不在关心写事件
                if(client_buffer[evs[i].data.fd].empty())
                {
                    ev.data.fd=evs[i].data.fd;
                    ev.events=EPOLLIN;
                    epoll_ctl(epoll_fd,EPOLL_CTL_MOD,ev.data.fd,&ev);
                }
            }
        }
    }
    

    return 0;
}

/**
 * @brief 帮助信息
 */
void _help()
{
    printf("\n");
    printf("Using: ./reverse_proxy_local log_file ip port\n");
    printf("Example: ./reverse_proxy_local /root/C++/selfC++Framework/log/reverse_proxy_local.log 192.168.59.132 5001\n");
    printf("\t/root/C++/selfC++Framework/tools/cpp/procctl 4 ./reverse_proxy_local /root/C++/selfC++Framework/log/reverse_proxy_local.log 192.168.59.132 5001\n");
    printf("parameters: \n");
    printf("\tlog_file  本程序运行的日志文件。\n");
    printf("\tip        外网代理程序的ip地址。\n");
    printf("\tport      外网代理程序的通讯端口。\n");
    printf("brief: \n");
    printf("\t本程序为tcp反向代理-内网端示例程序\n");
}

/**
 * @brief 向目标地址和端口发起socket连接
 * @param ip 目标地址
 * @param port 目标端口
 * @param is_block 是否阻塞
*/
int connectToDest(const char *ip, const int port, bool is_block)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        log_file.write("connectToDest():socket() failed.\n");
        return -1;
    }

    if(is_block == false)
    {
        if(setNonBlock(sock) < 0)
        {
            log_file.write("connectToDest():setNonBlock(%d) failed.\n", sock);
            close(sock);
            return -1;
        }
    }

    struct hostent* h = gethostbyname(ip);
    if(h == nullptr)
    {
        log_file.write("connectToDest():gethostbyname(%s) failed.\n", ip);
        close(sock);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, h->h_addr, h->h_length);

    int ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0 && errno != EINPROGRESS)
    {
        log_file.write("connectToDest():connect error, errno = %d, %s\n", errno, strerror(errno));
        close(sock);
        return -1;
    }
    return sock;
}

/**
 * @brief 进程退出函数
 * @param sig 信号值
*/
void EXIT(int sig)
{
    log_file.write("Process exit, sig = %d\n", sig);

    for(auto sock : client_sockets)
        if(sock > 0)
            close(sock);

    close(cmd_connect_socket);
    close(epoll_fd);
    close(timer_fd);

    exit(0);
}

/**
 * @brief 设置套接字为非阻塞模式
 * @param fd socket套接字描述符
*/
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