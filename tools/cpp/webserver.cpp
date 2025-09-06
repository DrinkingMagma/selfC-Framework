/**
 * @brief 数据访问结构模块（数据访问接口模块）
*/

#include "_public.h"
#include "_ooci.h"
using namespace idc;

#define _MAX_CLIENT_NUM 10
#define _MAX_RECV_BUFFER_SIZE 1024
#define _MAX_SEND_BUFFER_SIZE 1024

clogfile logfile;

void _help();
void EXIT(int sig);
int init_server(const int port);
bool get_value(const string &get_context, const string &name, string &value);

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
            int epoll_fd = epoll_create1(0);
            if (epoll_fd < 0) 
            {
                logfile.write("recv_func(): epoll_create1() failed: %s\n", strerror(errno));
                return;
            }

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
                        fcntl(client_sock, F_SETFL, fcntl(client_sock,F_GETFL,0) | O_NONBLOCK);
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
                        if((buffer_len = recv(evs[i].data.fd, buffer, sizeof(buffer), 0)) <= 0)
                        {
                            logfile.write("recv_func(): client(%d) disconnected.\n", evs[i].data.fd);
                            close(evs[i].data.fd);      // 关闭客户端连接
                            m_client_map.erase(evs[i].data.fd);     // 从状态机中删除客户端
                            continue;
                        }

                        logfile.write("recv_func(): recv %d bytes data from client(%d).\n", buffer_len, evs[i].data.fd);

                        // 将读取到的数据追加到对应socket的recv_buffer中
                        m_client_map[evs[i].data.fd].recv_buffer.append(buffer, buffer_len);

                        // 若recvbuffer中的内容以"\r\n\r\n"结束，则说明接收到了完整的HTTP请求
                        if(m_client_map[evs[i].data.fd].recv_buffer.compare(m_client_map[evs[i].data.fd].recv_buffer.size() - 4, 4, "\r\n\r\n") == 0)
                        {
                            logfile.write("recv_func(): recv a complete HTTP request from client(%d).\n", evs[i].data.fd);

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
            // 创建接收报文对象
            shared_ptr<ST_RecvOrSendMessage> ptr = make_shared<ST_RecvOrSendMessage>(sock, message);

            // 申请加锁
            lock_guard<mutex> lock(m_recv_queue_mutex);
            // 将接收报文对象放入接收队列
            m_recv_queue.push(ptr);
            // 通知工作线程处理接收队列
            m_recv_queue_cond.notify_one();
        }

        /**
         * @brief 工作线程主函数，处理接收队列中的请求报文
         * @param id 工作线程的id（仅用于调试和日志）
        */
        void work_func(int id)
        {
            Connection conn;

            if(conn.connecttodb("idc/idcpwd@ORCL", "Simplified Chinese_China.AL32UTF8") != 0)
            {
                logfile.write("connect database(idc/idcpwd@ORCL) failed: %s\n", conn.message());
                return;
            }

            while (true)
            {
                shared_ptr<ST_RecvOrSendMessage> ptr;

                {
                    // 将互斥锁转换为unique_lock<mutex>并申请加锁
                    unique_lock<mutex> lock(m_recv_queue_mutex);
                    // 若队列为空，进入循环，等待条件变量
                    while (m_recv_queue.empty())
                    {   
                        // 等待生产者的唤醒信号
                        m_recv_queue_cond.wait(lock);

                        if(m_is_exit == true)
                        {
                            logfile.write("work_func(): work thread(%d) will exit.\n", id);
                            return;
                        }
                    }
                    // 元素出队
                    ptr = m_recv_queue.front();
                    m_recv_queue.pop();
                }

                // 处理出队元素，即处理客户端的请求报文
                logfile.write("work_func(): work thread(%d) process request(socket = %d, message = \n%s).\n", id, ptr->sock, ptr->message.c_str());
                // logfile.write("work_func(): work thread(%s) process request(socket = %s).\n", id, ptr->sock);

                // 解析请求报文、判断权限、执行查询数据的sql语句、生成响应报文
                string send_buffer;
                handle_client_message(conn, ptr->message, send_buffer);
                string message = s_format(
                    "HTTP/1.1 200 OK\r\n"
                    "Server: webserver\t\n"
                    "Content-Type: text/html;charset=utf-8\r\n") + s_format("Content-Length: %d\r\n\r\n", send_buffer.size()) + send_buffer;

                logfile.write("work_func(): work thread(%d) response request(socket = %d, message = \n%s)\n", id, ptr->sock, send_buffer.c_str());
                // 将客户端socket和响应报文加入发送队列
                add_to_send_queue(ptr->sock, message);
            }
        }

        /**
         * @brief 处理客户端的请求报文，并生成响应报文
         * @param conn 数据库连接对象
         * @param recv_buffer http请求报文
         * @param send_buffer http响应报文的数据部分，不包括状态行和头部信息
        */
        void handle_client_message(Connection &conn, string &recv_buffer, string &send_buffer)
        {
            // 从请求报文中解析用户名、密码和接口名
            string username, password, intername;
            get_value(recv_buffer, "username", username);
            get_value(recv_buffer, "password", password);
            get_value(recv_buffer, "intername", intername);

            // 验证用户名和密码是否正确
            SqlStatement stmt(&conn);
            stmt.prepare("select ip from T_USERINFO where username=:1 and password=:2 and rsts=1");
            stmt.bindin(1, username);
            stmt.bindin(2, password);
            string ip;
            stmt.bindout(1, ip, 50);
            stmt.execute();
            if(stmt.next() != 0)
            {
                send_buffer = "<retcode>-1</retcode><message>用户名或密码错误</message>";
                return;
            }

            // 判断客户端连上来的地址是否与服务器数据库中保存的ip一致
            if(ip.empty() == false)
            {
                // ?
            }

            // 判断用户是否有访问接口的权限
            stmt.prepare("select count(*) from T_USERANDINTER "
                            "where username=:1 and intername=:2 and intername in (select intername from T_INTERCFG where rsts=1)");
            stmt.bindin(1,username);
            stmt.bindin(2,intername);
            int count = 0;
            stmt.bindout(1,count);
            stmt.execute();
            stmt.next();
            if(count == 0)
            {
                send_buffer = "<retcode>-1</retcode><message>用户无权限或接口不存在</message>";
                return;
            }

            // 根据接口名称，获取接口的配置参数
            string selectsql, colstr, bindin;
            stmt.prepare("select selectsql, colstr, bindin from T_INTERCFG where intername=:1");
            stmt.bindin(1, intername);
            stmt.bindout(1, selectsql);
            stmt.bindout(2, colstr);
            stmt.bindout(3, bindin);
            stmt.execute();
            if(stmt.next() != 0)
            {
                send_buffer = "<retcode>-1</retcode><message>内部错误</message>";
                return;
            }

            // 准备查询数据的sql语句
            stmt.prepare(selectsql);

            // 根据bindin字段，从请求报文中解析参数值
            ccmdstr cmdstr;
            cmdstr.split_to_cmd(bindin, ",");
            vector<string> v_bindin_value;
            v_bindin_value.resize(cmdstr.size());
            // 从请求报文中解析输入参数并绑定到sql语句
            for(int i = 0; i < cmdstr.size(); i++)
            {
                get_value(recv_buffer, cmdstr[i], v_bindin_value[i]);
                stmt.bindin(i + 1, v_bindin_value[i]);
            }

            // 绑定查询数据的sql语句的输出变量
            cmdstr.split_to_cmd(colstr, ",");

            // 存放结果集的数组
            vector<string> v_bindout_value;
            v_bindout_value.resize(cmdstr.size());
            // 将结果集绑定到数组上
            for(int i = 0; i < cmdstr.size(); i++)
            {
                stmt.bindout(i + 1, v_bindout_value[i]);
            }

            if(stmt.execute() != 0)
            {
                logfile.write("handle_client_message(): stmt.execute() failed: %s\n", stmt.message());
                s_format(send_buffer, "<retcode>%d</retcode><message>%s</message>", stmt.rc(), stmt.message());
                return;
            }

            send_buffer = "<retcode>0</retcode><message>ok</message>";
            send_buffer += "<data>\n";

            // 拼接结果集
            while (true)
            {
                if(stmt.next() != 0)
                    break;

                for(int i = 0; i < cmdstr.size(); i++)    
                {
                    send_buffer.append(s_format("<%s>%s</%s>", cmdstr[i].c_str(), v_bindout_value[i].c_str(), cmdstr[i].c_str()));
                }
                send_buffer.append("<endl/>\n");
            }
            send_buffer.append("</data>\n");

            logfile.write("handle_client_message(): intername = %s, count = %d\n", intername.c_str(), stmt.rpc());
            
            //将接口调用日志写入接口调用日志表T_USERLOG
            // ?
        }

        /**
         * @brief 将客户端的socket和响应报文放入发送队列中
         * @param sock 客户端socket
         * @param message 响应报文
        */
        void add_to_send_queue(int sock, string &message)
        {
            {
                shared_ptr<ST_RecvOrSendMessage> ptr = make_shared<ST_RecvOrSendMessage>(sock, message);
                lock_guard<mutex> lock(m_send_queue_mutex);
                m_send_queue.push(ptr);
            }
            // 通知发送线程处理发送队列中的数据
            write(m_send_pipe[1], (char*)"o", 1);
        }

        /**
         * @brief 发送线程主函数，将发送队列中的数据发送给客户端
        */
        void send_func()
        {
            // 创建epoll句柄
            int epoll_fd = epoll_create1(0);
            if (epoll_fd < 0) 
            {
                logfile.write("send_func(): epoll_create1() failed: %s\n", strerror(errno));
                return;
            }
            struct epoll_event ev;

            // 将发送队列的管道加入epoll
            ev.data.fd = m_send_pipe[0];
            ev.events = EPOLLIN;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);

            struct epoll_event evs[_MAX_SEND_BUFFER_SIZE];

            while(true)
            {
                int cur_fds = epoll_wait(epoll_fd, evs, _MAX_SEND_BUFFER_SIZE, -1);

                if(cur_fds < 0)
                {
                    logfile.write("send_func(): epoll_wait() failed.\n");
                    return;
                }

                for(int i = 0; i < cur_fds; i++)
                {
                    logfile.write("send_func(): cur fd = %d(event = %d)\n", evs[i].data.fd, evs[i].events);

                    // 若管道发生事件，表示发送队列有报文需要发送
                    if(evs[i].data.fd == m_send_pipe[0])
                    {
                        if(m_is_exit == true)
                        {
                            logfile.write("send_func(): send thread will exit.\n");
                            return;
                        }

                        char c;
                        read(m_send_pipe[0], &c, 1);

                        shared_ptr<ST_RecvOrSendMessage> ptr;
                        lock_guard<mutex> lock(m_send_queue_mutex);

                        while(m_send_queue.empty() == false)
                        {
                            ptr = m_send_queue.front();
                            m_send_queue.pop();

                            // 将出队的报文保存到socket的发送缓冲区中
                            m_client_map[ptr->sock].send_buffer.append(ptr->message);

                            // 监听客户端socket的可写事件
                            ev.data.fd = ptr->sock;
                            ev.events = EPOLLOUT;
                            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
                        }
                        continue;
                    }

                    // 若客户端socket可写事件发生，表示有数据可发送
                    if(evs[i].events & EPOLLOUT)
                    {
                        // 将响应报文发送给客户端
                        int write_len = send(evs[i].data.fd, m_client_map[evs[i].data.fd].send_buffer.data(), m_client_map[evs[i].data.fd].send_buffer.length(), 0);
                        logfile.write("send_func(): send thread send %d bytes to %d\n", write_len, evs[i].data.fd);

                        // 删除已成功发送的数据
                        m_client_map[evs[i].data.fd].send_buffer.erase(0, write_len);

                        // 若缓冲区为空，则删除监听可写事件
                        if(m_client_map[evs[i].data.fd].send_buffer.empty())
                        {
                            ev.data.fd = evs[i].data.fd;
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev);
                        }
                    }
                }
            }
        }
};


C_Thread thread_instance;

int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        _help();
        return -1;
    }

    close_io_and_signal();
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if(logfile.open(argv[1]) == false)
    {
        printf("logfile.open(%s) failed.\n", argv[1]);
        return -1;
    }

    // 创建接收线程、工作线程和发送线程
    thread t_1(&C_Thread::recv_func, &thread_instance, atoi(argv[2]));
    thread t_2(&C_Thread::work_func, &thread_instance, 1);
    thread t_4(&C_Thread::work_func, &thread_instance, 2);
    thread t_5(&C_Thread::work_func, &thread_instance, 3);
    thread t_6(&C_Thread::send_func, &thread_instance);

    logfile.write("main(): all threads have been started, start working...\n");

    while (true)
    {
        sleep(30);
        // 可以执行一些定时任务
    }

    return 0; 
}

/**
 * @brief 程序帮助信息
*/
void _help()
{
    print_dash_line(60);
    printf("Usage: /root/C++/selfC++Framework/tools/bin/webserver log_file_name port\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/webserver /root/C++/selfC++Framework/log/webserver.log 5088\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 5 /root/C++/selfC++Framework/tools/bin/webserver /root/C++/selfC++Framework/log/webserver.log 5088\n");
    printf("brief: 数据访问接口模块\n");
    printf("Param: \n");
    printf("log_file_name 日志文件\n");
    printf("port 监听端口\n");
    print_dash_line(60);
}

/**
 * @brief 退出信号处理函数
 * @param sig 退出信号
*/
void EXIT(int sig)
{
    signal(sig, SIG_IGN);
    logfile.write("Program exit, sig = %d\n", sig);

    // 通知接收线程退出
    write(thread_instance.m_recv_pipe[1], (char*)"o", 1);
    // 保证线程们有足够时间退出
    usleep(500);
    exit(0);
}

/**
 * @brief 初始化服务端的监听端口
 * @param port 监听端口
*/
int init_server(const int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        logfile.write("init_server(): socket(%d) failed.\n", port);
        return -1;
    }

    int opt = 1;
    unsigned int len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        logfile.write("init_server(): bind(%d) failed.\n", port);
        return -1;
    }

    if(listen(sock, 5) != 0)
    {
        logfile.write("init_server(): listen(%d) failed.\n", port);
        return -1;
    }

    // 设置socket为非阻塞模式
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    return sock;
}

/**
 * @brief 从Get请求中获取参数的值
 * @param get_context Get请求报文的内容
 * @param name 参数名称
 * @param value 参数的值
*/
bool get_value(const string &get_context, const string &name, string &value)
{
    string::size_type start_position = get_context.find(name);

    if(start_position == string::npos)
        return false;

    string::size_type end_position = get_context.find("&", start_position);

    // 若没有找到符号"&"，则将end_position设置为空格
    if(end_position == string::npos)
        end_position = get_context.find(" ", start_position);

    if(end_position == string::npos)
        return false;

    value = get_context.substr(start_position + name.length() + 1, end_position - start_position - name.length() - 1);

    return true;
}