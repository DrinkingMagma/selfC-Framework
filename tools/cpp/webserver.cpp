/**
 * @brief 数据访问结构模块
*/

#include "_public.h"
#include "_ooci.h"
using namespace idc;

#define _MAX_CLIENT_NUM 10
#define _MAX_RECV_BUFFER_SIZE 1024

clogfile logfile;
C_Thread thread;

// 客户端的结构体
struct ST_Client
{
    string client_ip;       // 客户端IP
    int client_atime = 0;   // 客户端最后活跃时间
    string recv_buffer;     // 客户端的接收缓冲区
    string send_buffer;     // 客户端的发送缓冲区
};

// 接收/发送队列的结构体
struct ST_RecvOrSendMessage
{
    int sock = 0;
    string message;

    ST_RecvOrSendMessage(int in_sock, string in_message) : sock(in_sock), message(in_message)
    {
        logfile.write("%d create a new message.\n", sock);
    };
};

// 线程类
class C_Thread
{
    private:
        queue<shared_ptr<ST_RecvOrSendMessage>> m_recv_queue;       // 接收队列
        mutex m_recv_queue_mutex;                                   // 接收队列的互斥锁
        condition_variable m_recv_queue_cond;                       // 接收队列的条件变量

        queue<shared_ptr<ST_RecvOrSendMessage>> m_send_queue;       // 发送队列
        mutex m_send_queue_mutex;                                   // 发送队列的互斥锁
        int m_send_pipe[2] = {0};                                   // 工作线程通知发送线程的管道

        unordered_map<int, struct ST_Client> m_client_map;          // 存放客户端对象的哈希表，俗称状态机

        atomic_bool m_is_exit;                                      // 工作线程和发送线程是否退出
    public:
        int m_recv_pipe[2] = {0};                                   // 主进程通知接收线程退出的管道，因为主进程需要用到该成员，所以声明为public

        /**
         * @brief 构造函数
        */
        C_Thread()
        {
            pipe(m_send_pipe);
            pipe(m_recv_pipe);
            m_is_exit = false;
        }

        /**
         * @brief 接收线程主函数
         * @param listen_port 监听的端口
        */
        void recv_func(int listen_port)
        {
            // 初始化服务端用于监听的socket
            int listen_sock = init_server(listen_port);
            if(listen_sock < 0)
            {
                logfile.write("recv_func(): init_server(%d) failed\n", listen_port);
            }

            // 创建epoll句柄
            int epoll_fd = epoll_create(0);

            // 为监听的socket准备读事件
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = listen_sock;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);

            // 将接收主进程通知的管道加入epoll中
            ev.data.fd = m_recv_pipe[0];
            ev.events = EPOLLIN;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);

            struct epoll_event evs[_MAX_CLIENT_NUM];

            while (true)
            {
                int cur_fds = epoll_wait(epoll_fd, evs, _MAX_CLIENT_NUM, -1);

                if(cur_fds < 0)
                {
                    logfile.write("recv_func(): epoll_wait() failed.\n");
                    return;
                }

                for(int i = 0; i < cur_fds; i++)
                {
                    logfile.write("recv_func(): recv event, fd = %d, events = %d.\n", evs[i].data.fd, evs[i].events);

                    // 若listen_sock发生事件，说明有新的客户端连接
                    if(evs[i].data.fd == listen_sock)
                    {
                        struct sockaddr_in client_addr;
                        socklen_t client_len = sizeof(client_addr);
                        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);

                        // 将新的socket设置为非阻塞模式
                        fcntl(client_sock, F_SETFL, fcntl(client_sock,F_GETFD,0) | O_NONBLOCK);
                        logfile.write("recv_func(): accept new client(socket = %d) succeeded.\n", client_sock);

                        // 保存客户端的ip地址和最后活跃时间
                        m_client_map[client_sock].client_ip = inet_ntoa(client_addr.sin_addr);
                        m_client_map[client_sock].client_atime = time(0);

                        // 为新的客户端连接准备读事件，并添加到epoll中
                        ev.data.fd = client_sock;
                        ev.events = EPOLLIN;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev);

                        continue;
                    }

                    // 接收管道发生可读事件
                    if(evs[i].data.fd == m_recv_pipe[0])
                    {
                        logfile.write("recv_func(): recv thread will exit.\n");

                        m_is_exit = true;
                        m_recv_queue_cond.notify_all();     // 通知所有工作线程退出
                        write(m_send_pipe[1], (char*)"o", 1);    // 通知发送线程退出
                        return;
                    }

                    // 若不是以上两种情况，则只能是客户端socket事件，此时有两种可能
                    // 1）客户端发送报文；2）客户端断开连接
                    // 若是可读事件，说明是可能1
                    if(evs[i].events & EPOLLIN)
                    {
                        char buffer[5000];
                        int buffer_len = 0;

                        // 若读取报文失败，说明连接断开
                        if(buffer_len = recv(evs[i].data.fd, buffer, sizeof(buffer), 0) <= 0)
                        {
                            logfile.write("recv_func(): client(%s) disconnected.\n", evs[i].data.fd);
                            close(evs[i].data.fd);      // 关闭客户端连接
                            m_client_map.erase(evs[i].data.fd);     // 从状态机中删除客户端
                            continue;
                        }

                        logfile.write("recv_func(): recv %d bytes data from client(%s).\n", buffer_len, evs[i].data.fd);

                        // 将读取到的数据追加到对应socket的recv_buffer中
                        m_client_map[evs[i].data.fd].recv_buffer.append(buffer, buffer_len);

                        // 若recvbuffer中的内容以"\r\n\r\n"结束，则说明接收到了完整的HTTP请求
                        if(m_client_map[evs[i].data.fd].recv_buffer.compare(m_client_map[evs[i].data.fd].recv_buffer.size() - 4, 4, "\r\n\r\n") == 0)
                        {
                            logfile.write("recv_func(): recv a complete HTTP request from client(%s).\n", evs[i].data.fd);

                            // 将完整的请求报文入队，交给工作线程
                            add_to_recv_queue((int)evs[i].data.fd, m_client_map[evs[i].data.fd].recv_buffer);
                            // 清空socket对应的recv_buffer缓冲区
                            m_client_map[evs[i].data.fd].recv_buffer.clear();
                        }
                        else
                        {
                            if(m_client_map[evs[i].data.fd].recv_buffer.size() > _MAX_RECV_BUFFER_SIZE)
                            {
                                close(evs[i].data.fd);
                                m_client_map.erase(evs[i].data.fd);;
                                // 可以考虑将客户端ip拉入黑名单
                            }
                        }
                        // 更新对应客户端的最后活跃时间
                        m_client_map[evs[i].data.fd].client_atime = time(0);
                    }
                }
            }
        }

        /**
         * @brief 将客户端socket和请求报文放入接收队列
         * @param sock 客户端socket
         * @param message 客户端的请求报文
        */
        void add_to_recv_queue(int sock, string &message)
        {

        }

        /**
         * @brief 工作线程主函数，处理接收队列中的请求报文
         * @param id 工作线程的id（仅用于调试和日志）
        */
        void work_func(int id)
        {

        }

        /**
         * @brief 处理客户端的请求报文，并生成响应报文
         * @param conn 数据库连接对象
         * @param recv_buffer http请求报文
         * @param send_buffer http响应报文的数据部分，不包括状态行和头部信息
        */
        void handle_client_message(Connection &conn, string &recv_buffer, string &send_buffer)
        {

        }

        /**
         * @brief 将客户端的socket和响应报文放入发送队列中
         * @param sock 客户端socket
         * @param message 响应报文
        */
        void add_to_send_queue(int sock, string &message)
        {

        }

        /**
         * @brief 发送线程主函数，将发送队列中的数据发送给客户端
        */
        void send_func()
        {

        }
};

void _help();
void EXIT(int sig);
int init_server(const int port);
bool get_value(const string &get_context, const string &name, string &value);

int main(int argc,char *argv[])
{
    return true; 
}

/**
 * @brief 程序帮助信息
*/
void _help()
{

}

/**
 * @brief 退出信号处理函数
 * @param sig 退出信号
*/
void EXIT(int sig)
{

}

/**
 * @brief 初始化服务端的监听端口
 * @param port 监听端口
*/
int init_server(const int port)
{
    return 1;
}

/**
 * @brief 从Get请求中获取参数的值
 * @param get_context Get请求报文的内容
 * @param name 参数名称
 * @param value 参数的值
*/
bool get_value(const string &get_context, const string &name, string &value)
{

    return true;
}