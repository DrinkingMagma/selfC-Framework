#include "_public.h"
using namespace idc;

// 程序运行的参数结构体
struct st_arg{
    int  client_type;               // 客户端类型：1-下载文件；2-上传文件
    char server_path[256];          // 服务器端文件的存放路径
    int file_process_type;          // 文件下载成功后原文件的处理方式：1-不处理；2-删除；3-备份
    char server_path_bak[256];      // 服务器端文件的备份路径
    bool and_child;                 // 是否下载子目录下的文件
    char match_rules[256];          // 待下载文件名的匹配规则
    char client_path[256];          // 客户端存放文件的地址
    int time_tvl;                   // 扫描服务端目录文件的时间间隔。单位：秒
    int time_out;                   // 进程心跳的超时时间
    char process_name[51];          // 进程名，用于心跳检测
} arg;

clogfile log_file;
ctcpserver tcp_server;
string g_str_send_buffer;
string g_str_recv_buffer;
cpactive pactive;

void _help();
void FatherEXIT(int sig);
void ChilderEXIT(int sig);
bool cilentLogin();
void recvFilesMain();
bool recvFile(const string &file_name, const string &file_mtime, int file_size);
bool _tcpPutFiles(bool &bcontinue);
void sendFilesMain();
bool sendFile(const string &file_name, const int file_size);
bool activeTest();
bool ackMessage(const string &str_recv_buffer);

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        _help();
        return -1;
    }

    signal(SIGINT, FatherEXIT);
    signal(SIGTERM, FatherEXIT);

    if(log_file.open(argv[2], ios::app) == false)
    {
        printf("log_file.open(%s) failed.\n", argv[2]);
        return -1;
    }

    log_file.write("---------------------------------\n");
    if(tcp_server.init_server(atoi(argv[1])) == false)
    {
        log_file.write("tcp_server.init_server(%s) failed.\n", argv[1]);
        return -1;
    }

    log_file.write("tcp_server.init_server(%s) success.\n", argv[1]);
    while(true)
    {
        if(tcp_server.accept_client() == false)
        {
            log_file.write("tcp_server.accept_client() failed.\n");
            FatherEXIT(-1);
        }

        log_file.write("accept a client(%s).\n", tcp_server.get_ip());

        // 大于0表示为父进程
        if(fork() > 0)
        {
            tcp_server.close_client();
            continue;
        }

        // 以下为子进程运行代码
        // -------------------------------------------------------------
        signal(SIGINT, ChilderEXIT);
        signal(SIGTERM, ChilderEXIT);

        tcp_server.close_listen();

        if(cilentLogin() == false)
        {
            log_file.write("login failed.\n");
            ChilderEXIT(-1);
        }

        pactive.add_p_info(arg.time_out, arg.process_name);

        if(arg.client_type == 1)
            sendFilesMain();
        else if(arg.client_type == 2)
            recvFilesMain();
        else
            log_file.write("client_type error.\n");

        ChilderEXIT(0);
    }

    return 0;
}

void _help()
{
    printf("\n"); 
    printf("Usage: /root/C++/selfC++Framework/tools/bin/tcpserver port log_file\n");
    printf("Example: /root/C++/selfC++Framework/tools/bin/tcpserver 5005 /root/C++/selfC++Framework/log/tcpserver.log\n");
    printf("/root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/tcpserver 5005 /root/C++/selfC++Framework/log/tcpserver.log\n");
}

void FatherEXIT(int sig)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    log_file.write("Father process exit, sig = %d.\n", sig);
    tcp_server.close_listen();
    kill(0, 15);       // 通知全部子进程退出

    exit(0);
}

void ChilderEXIT(int sig)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    log_file.write("Child process exit, sig = %d.\n", sig);
    tcp_server.close_client();

    exit(0);
}

bool cilentLogin()
{
    if(tcp_server.read(g_str_recv_buffer, 10) == false)
    {
        log_file.write("g_str_recv_buffer: %s\n", g_str_recv_buffer.c_str());
        log_file.write("tcp_server.read():read login message failed.\n");
        return false;
    }

    memset(&arg, 0, sizeof(arg));
    get_xml_buffer(g_str_recv_buffer, "client_type", arg.client_type);
    get_xml_buffer(g_str_recv_buffer, "client_path", arg.client_path);
    get_xml_buffer(g_str_recv_buffer, "server_path", arg.server_path);
    get_xml_buffer(g_str_recv_buffer, "server_path_bak", arg.server_path_bak);
    get_xml_buffer(g_str_recv_buffer, "and_child", arg.and_child);
    get_xml_buffer(g_str_recv_buffer, "file_process_type", arg.file_process_type);
    get_xml_buffer(g_str_recv_buffer, "match_rules", arg.match_rules);
    get_xml_buffer(g_str_recv_buffer, "time_tvl", arg.time_tvl);
    get_xml_buffer(g_str_recv_buffer, "time_out", arg.time_out);
    get_xml_buffer(g_str_recv_buffer, "process_name", arg.process_name, 50);

    if(arg.client_type != 1 && arg.client_type != 2)
        g_str_send_buffer = "client_type error";
    else
        g_str_send_buffer = "success";

    if(tcp_server.write(g_str_send_buffer) == false)
    {
        log_file.write("tcp_server.write():send login result message failed.\n");
        return false;
    }
    log_file.write("Tcp client: %s login %s.\n", tcp_server.get_ip(), g_str_send_buffer.c_str());

    return true;
}

void recvFilesMain()
{
    while(true)
    {
        pactive.upt_atime();

        // 接受服务器报文
        if(tcp_server.read(g_str_recv_buffer, arg.time_tvl + 10) == false)
        {
            log_file.write("tcp_server.read(): recv server message failed.\n");
            return;
        }

        // 处理心跳报文
        if(g_str_recv_buffer == "<active_test>ok</active_test>")
        {
            g_str_send_buffer = "ok";

            if(tcp_server.write(g_str_send_buffer) == false)
            {
                log_file.write("tcp_server.write(): processing pactive message failed.\n");
                return;
            }
        }

        // 处理上传文件的请求报文
        if(g_str_recv_buffer.find("<file_name>") != string::npos)
        {
            string client_file_name;
            string client_file_mtime;
            int client_file_size;

            get_xml_buffer(g_str_recv_buffer, "file_name", client_file_name);
            get_xml_buffer(g_str_recv_buffer, "file_mtime", client_file_mtime);
            get_xml_buffer(g_str_recv_buffer, "file_size", client_file_size);

            // 获取服务器文件地址
            string server_file_name = client_file_name;
            replace_str(server_file_name, arg.client_path, arg.server_path, false);

            // 接受文件内容
            log_file.write("recv file(%s) from (%s) ...... ", server_file_name.c_str(), client_file_name.c_str());
            if(recvFile(server_file_name, client_file_mtime, client_file_size) == true)
            {
                log_file << "success.\n";
                s_format(g_str_send_buffer, "<file_name>%s</file_name><result>success</result>", client_file_name.c_str());
            }
            else
            {
                log_file << "failed.\n";
                s_format(g_str_send_buffer, "<file_name>%s</file_name><result>failed</result>", client_file_name.c_str());
            }

            // 返回文件下载结果
            if(tcp_server.write(g_str_send_buffer) == false)
            {
                log_file.write("tcp_server.write(): return the result of file download failed.\n");
                return;
            }
        }
    }
}

bool recvFile(const string &file_name, const string &file_mtime, int file_size)
{
    int total_bytes = 0;
    int on_read = 0;
    char buffer[1000];
    cofile ofile;

    new_dir(file_name);

    if(ofile.open_file(file_name, true, ios::out | ios::binary) == false)
    {
        log_file.write("ofile.open_file(%s) failed.\n", file_name.c_str());
        return false;
    }

    while(true)
    {
        memset(buffer, 0, sizeof(buffer));

        if((size_t)(file_size - total_bytes) > sizeof(buffer))
            on_read = sizeof(buffer);
        else
            on_read = file_size - total_bytes;

        if(tcp_server.read(buffer, on_read) == false)
        {
            log_file.write("tcp_server.read():downloading file failed.\n");
            return false;
        }

        ofile.write(buffer, on_read);

        total_bytes += on_read;

        if (total_bytes == file_size)
            break;
    }

    ofile.close_and_rename();

    set_mtime(file_name, file_mtime);

    return true;
}

bool _tcpPutFiles(bool &bcontinue)
{
    bcontinue = false;

    cdir dir;

    if(dir.open_dir(arg.server_path, arg.match_rules, 10000, arg.and_child) == false)
    {
        log_file.write("dir.open_dir(%s, %s, 10000, %s) failed.\n", arg.client_path, arg.match_rules, arg.and_child ? "true" : "false");
        return false;
    }

    // 上传文件后，未收到确认报文的文件数量
    int delayed = 0;

    while(dir.read_dir())
    {
        bcontinue = true;

        s_format(g_str_send_buffer, "<file_name>%s</file_name><file_mtime>%s</file_mtime><file_size>%d</file_size>",
                                    dir.m_ffilename.c_str(), dir.m_mtime.c_str(), dir.m_filesize);
        if(tcp_server.write(g_str_send_buffer) == false)
        {
            log_file.write("send file(%s) to server failed.\n", dir.m_ffilename.c_str());
            return false;
        }

        log_file.write("send file(%s) ...... ", dir.m_ffilename.c_str());
        if(sendFile(dir.m_ffilename, dir.m_filesize) == true)
        {
            log_file << "success.\n";
            delayed++;
        }
        else
        {
            log_file << "failed.\n";
            tcp_server.close_client();
            return false;
        }

        pactive.upt_atime();

        while(delayed > 0)
        {
            if(tcp_server.read(g_str_recv_buffer, 30) == false)
            {
                log_file.write("When sended file(%s), receive back info from client failed.\n", dir.m_ffilename.c_str());
                break;
            }

            delayed--;
            ackMessage(g_str_recv_buffer);
        }
    }

    while(delayed > 0)
    {
        if(tcp_server.read(g_str_recv_buffer, 30) == false)
        {
            
            log_file.write("receive back info from client failed.\n");
            break;
        }

        delayed--;
        ackMessage(g_str_recv_buffer);
    }

    bcontinue = dir.read_dir();
    return true;
}

void sendFilesMain()
{
    pactive.add_p_info(arg.time_out, arg.process_name);

    bool bcontinue = true;

    while(true)
    {
        if(_tcpPutFiles(bcontinue) == false)
        {
            log_file.write("_tcpPutFiles() failed.\n");
            return;
        }

        if(bcontinue == false)
        {
            sleep(arg.time_tvl);
            if(activeTest() == false)
                break;
        }

        pactive.upt_atime();
    }
}

bool sendFile(const string &file_name, const int file_size)
{
    int on_read = 0;
    char buffer[1000];
    int total_bytes = 0;
    cifile ifile;

    if(ifile.open_file(file_name, ios::in | ios::binary) == false)
    {
        log_file.write("open file(%s) failed.\n", file_name.c_str());
        return false;
    }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        if((size_t)(file_size - total_bytes) > sizeof(buffer))
            on_read = sizeof(buffer);
        else
            on_read = file_size - total_bytes;

        ifile.read(buffer, on_read);

        if(tcp_server.write(buffer, on_read) == false)
        {
            log_file.write("uploading file(%s) failed.\n", file_name);
            return false;
        }

        total_bytes += on_read;

        if(total_bytes == file_size)
            break;
    }
    return true;
}

bool activeTest()
{
    g_str_send_buffer = "<activate_test>ok</activate_test>";

    if(tcp_server.write(g_str_send_buffer) == false)
    {
        log_file.write("tcp_server.write():active test send message failed.\n");
        return false;
    }

    if(tcp_server.read(g_str_recv_buffer, 20) == false)
    {
        log_file.write("tcp_server.read():active test recv message failed.\n");
        return false;
    }

    return true;
}

bool ackMessage(const string &str_recv_buffer)
{
    string file_name;
    string upload_result;

    get_xml_buffer(g_str_recv_buffer, "file_name", file_name);
    get_xml_buffer(g_str_recv_buffer, "result", upload_result);

    if(upload_result != "success")
    {
        log_file.write("server recv file failed, upload result(%s).\n", upload_result.c_str());
        return false;
    }

    if(arg.file_process_type == 2)
    {
        if(remove(file_name.c_str()) != 0)
        {
            log_file.write("remove(%s) failed.\n", file_name.c_str());
            return false;
        }
    }

    if(arg.file_process_type == 3)
    {
        string bak_file_name = file_name;
        replace_str(bak_file_name, arg.server_path, arg.server_path_bak, false);
        if(rename_file(file_name, bak_file_name) == false)
        {
            log_file.write("rename_file(%s, %s) failed.\n", file_name.c_str(), bak_file_name.c_str());
            return false;
        }
    }

    return true;
}
