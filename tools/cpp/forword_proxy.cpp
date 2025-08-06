/**
 * @brief 正向网络代理服务器
*/
#include "_public.h"
using namespace idc;

// 代理路由参数的结构体
struct ST_Route
{
    int src_port;       // 源端口
    char dst_ip[31];    // 目标IP
    int dst_port;       // 目标端口
    int listen_socket;  // 源端口监听的socket
}st_route;

void _help();

vector<ST_Route> v_route;   // 代理路由参数的容器
bool loadRoute(const char* init_file);

int initServer(int port);

int epoll_fd = 0;       // epoll的句柄
#define MAX_EVS 1000    // epoll监听的最大事件数

#define MAX_SOCKETS 1024        // 最大连接数
int client_sockets[MAX_SOCKET_NUM];    // 保存所有连接的socket
int client_a_time[MAX_SOCKET_NUM];     // 保存所有连接的socket的最后活跃时间
string client_buffer[MAX_SOCKET_NUM];  // 保存所有连接的发送内容的缓存

clogfile log_file;   // 日志文件
cpactive pactive;   // 进程心跳

int connectToDest(const char *ip, const int port);
void EXIT(int sig);
int setNonBlock(int fd);

int main(int argc, char *argv[])
{
    if(argc != 3)
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
    pactive.add_p_info(30, "forword_proxy");

    if(loadRoute(argv[2]) == false)
    {
        log_file.write("load route file failed.\n");
        return -1;
    }
    log_file.write("---------------------new-----------------------------\n");
    log_file.write("load route file success, number is %d.\n", v_route.size());

    // 初始化服务端监听的所有socket
    for(auto &r : v_route)
    {
        r.listen_socket = initServer(r.src_port);
        if(r.listen_socket < 0)
        {
            log_file.write("init server failed, port is %d.\n", r.src_port);
            continue;
        }
    }

    // 创建epoll实例
    epoll_fd = epoll_create1(0);
    // 声明事件的数据结构
    struct epoll_event ev;

    // 监听每个socket的可读事件
    for(auto r : v_route)
    {
        if(r.listen_socket >= 0)
        {
            ev.events = EPOLLIN;
            ev.data.fd = r.listen_socket;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, r.listen_socket, &ev);
        }
    }

    // --------------------------------------------------------------------
    // 将定时器加入epoll中
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); // 创建定时器文件描述符
    struct itimerspec time_out;                     // 定时器结构体
    memset(&time_out, 0, sizeof(time_out));
    time_out.it_value.tv_sec = 10;                  // 定时器间隔10秒
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
            
            // 对所有监听的socket进行监听
            int j;
            for(j = 0; j < v_route.size(); ++j)
            {
                // 当各路由对应的socket有事件发生时
                if(evs[i].data.fd == v_route[j].listen_socket)
                {
                    // 接收客户端的连接
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int src_socket = accept(v_route[j].listen_socket, (struct sockaddr *)&client_addr, &client_len);

                    if(src_socket < 0)
                    {
                        log_file.write("accept source socket error: %s\n", strerror(errno));
                        break;
                    }
                    if(src_socket >= MAX_SOCKET_NUM)
                    {
                        log_file.write("when accept source socket, too many sockets(%d >= %d)\n", src_socket, MAX_SOCKET_NUM);
                        close(src_socket);
                        break;
                    }

                    // 向目标地址及端口发起连接
                    int dst_socket = connectToDest(v_route[j].dst_ip, v_route[j].dst_port);
                    if(dst_socket < 0)
                    {
                        log_file.write("connect connect destination error: %s\n", strerror(errno));
                        close(src_socket);
                        break;
                    }
                    if(dst_socket >= MAX_SOCKET_NUM)
                    {
                        log_file.write("when connect destination socket, too many sockets(%d > %d)\n", dst_socket, MAX_SOCKET_NUM);
                        close(src_socket);
                        close(dst_socket);
                        break;
                    }

                    log_file.write("accept socket by port(%d) and client (%d -> %d)\n", v_route[j].src_port, src_socket, dst_socket);

                    // 为两个socket准备读事件并添加到epoll中
                    ev.data.fd = src_socket;
                    ev.events = EPOLLIN;
                    setNonBlock(src_socket);
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, src_socket, &ev) < 0)
                    {
                        log_file.write("epoll_ctl add source socket error: %s\n", strerror(errno));
                        close(src_socket);
                        close(dst_socket);
                        break;
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

                    // 一个事件只会对应一个监听的socket，因此找到后不需要继续寻找
                    break;
                }
            }

            // 若事件已被处理，则跳过
            if(j < v_route.size())
                continue;

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
                    log_file.write("read error, close socket(%d - %d)\n", evs[i].data.fd, client_sockets[evs[i].data.fd]);
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
    printf("Using: ./forword_proxy log_file init_file\n");
    printf("Example: ./forword_proxy /root/C++/selfC++Framework/log/forword_proxy.log ./forword_proxy.conf\n");
    printf("\t/root/C++/selfC++Framework/tools/cpp/procctl 4 ./forword_proxy /root/C++/selfC++Framework/log/forword_proxy.log ./forword_proxy.conf\n");
    printf("parameters: \n");
    printf("\tlog_file   本程序运行的日志文件。\n");
    printf("\tinit_file  本程序的初始化文件。\n");
    printf("brief: \n");
    printf("\t本程序为tcp正向代理示例程序\n");
}

/**
 * @brief 将代理路由参数加载到v_route容器中
 * @param init_file 参数文件
*/
bool loadRoute(const char* init_file)
{
    cifile i_file;

    if(!i_file.open_file(init_file))
    {
        log_file.write("open %s failed.\n", init_file);
        return false;
    }

    string str_buffer;
    ccmdstr cmd_str;

    while (true)
    {
        if(i_file.read_line(str_buffer) == false)
            break;
        
        // 去除注释
        auto pos = str_buffer.find("#");
        if(pos != string::npos)
            str_buffer.resize(pos);

        // 去除前后空格
        delete_lrchr(str_buffer, ' ');
        // 将两个空格替换为一个空格（递归替换，用于匹配不同长度的端口号）
        replace_str(str_buffer, "  ", " ", true);

        cmd_str.split_to_cmd(str_buffer, " ");

        if(cmd_str.size() != 3)
            continue;

        memset(&st_route, 0, sizeof(st_route));
        cmd_str.getvalue(0, st_route.src_port);
        cmd_str.getvalue(1, st_route.dst_ip);
        cmd_str.getvalue(2, st_route.dst_port);

        v_route.push_back(st_route);
    }

    return true;
}

/**
 * @brief 初始化服务端
 * @param port 服务器端口
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

/**
 * @brief 向目标地址和端口发起socket连接
 * @param ip 目标地址
 * @param port 目标端口
*/
int connectToDest(const char *ip, const int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        log_file.write("connectToDest():socket() failed.\n");
        return -1;
    }

    if(setNonBlock(sock) < 0)
    {
        log_file.write("connectToDest():setNonBlock(%d) failed.\n", sock);
        close(sock);
        return -1;
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

    for(auto & r : v_route)
        if(r.listen_socket > 0)
            close(r.listen_socket);

    for(auto sock : client_sockets)
        if(sock > 0)
            close(sock);

    close(epoll_fd);

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