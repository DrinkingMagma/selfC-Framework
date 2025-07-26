#include "_public.h"
using namespace idc;

// 程序运行的参数结构体
struct st_arg{
    int  client_type;               // 客户端类型：1-上传文件；2-下载文件，本程序固定填2
    char ip[31];                    // 服务器端的IP地址
    int  port;                      // 服务器端的端口号
    char server_path[256];          // 服务器端文件的存放路径
    int file_process_type;          // 文件下载成功后原文件的处理方式：1-不处理；2-删除；3-备份
    char server_path_bak[256];      // 服务器端文件的备份路径
    bool and_child;                 // 是否下载子目录下的文件
    char match_rules[256];          // 待下载文件名的匹配规则
    char client_path[256];          // 
    char client_path_bak[256];
    int time_tvl;                   // 扫描服务端目录文件的时间间隔。单位：秒
    int time_out;                   // 进程心跳的超时时间
    char process_name[51];          // 进程名，用于心跳检测
} arg;

clogfile logfile;               // 日志对象
ctcpclient tcp_client;          // tcp客户端对象
string g_str_recv_buffer;       // 接受报文的缓冲区
string g_str_send_buffer;       // 发送报文的缓冲区
cpactive pactive;               // 进程心跳对象

void EXIT(int sig);
void _help();
bool _xmlToArg(char *xml_buffer);
bool login(const char *argv);
void _tcpGetFiles();
bool recvFile(const string &file_name, const string &mtime, int file_size);
bool _tcpPutFiles(bool &bcontinue);
bool sendFile(const string &file_name, const int file_size);
bool ackMessage(const string &str_recv_buffer);
bool activeTest();

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        _help();
        return -1;
    }

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if(logfile.open(argv[1]) == false)
    {
        printf("logfile.open(%s) failed.\n", argv[1]);
        return -1;
    }

    cifile ifile;
    if(ifile.open_file(argv[2]) == false)
    {
        logfile.write("ifile.open_file(%s) failed.\n", argv[2]);
        return -1;
    }

    char* str_xml_buffer = ifile.read_all();
    if(_xmlToArg(str_xml_buffer) == false)
    {
        logfile.write("_xmlToArg() failed.\n");
        return -1;
    }

    if(tcp_client.connect(arg.ip, arg.port) == false)
    {
        logfile.write("tcp_client.connect(%s, %d) failed.\n", arg.ip, arg.port);
        EXIT(-1);
    }

    if(login(str_xml_buffer) == false)
    {
        logfile.write("login() failed.\n");
        EXIT(-1);
    }

    if(arg.client_type == 2)
        _tcpGetFiles();
    else
    {
        bool bcontinue = true;
        while(true)
        {
            if(_tcpPutFiles(bcontinue) == false)
            {
                logfile.write("_tcpPutFiles() failed.\n");
                EXIT(-1);
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

    return 0;
}

/**
 * @brief 退出信号处理函数
 * @param sig 退出信号
*/
void EXIT(int sig)
{
    logfile.write("Process exit, sig: %d\n", sig);

    exit(0);
}

/**
 * @brief 帮助函数，显示使用方法
*/
void _help()
{
    printf("\n");
    printf("Usage: /root/C++/selfC++Framework/tools/bin/tcpclientfiles log_file_name xml_file_name\n");
    printf("Examplr: /root/C++/selfC++Framework/tools/bin/procctl 20 /root/C++/selfC++Framework/tools/bin/tcpclientfiles /root/C++/selfC++Framework/log/tcpclientfiles.log /root/C++/selfC++Framework/tools/cpp/tcpclientfiles_config.xml\n");
    printf("本程序采用tcp协议从服务端下载/上传文件。\n");
    printf("log_file_name   本程序运行的日志文件。\n");
    printf("xml_buffer     本程序运行的参数，如下：\n");
    printf("client_type         客户端类型：1-下载文件；2-上传文件。\n");
    printf("ip                  服务端的IP地址。\n");
    printf("port                服务端的端口。\n");
    printf("file_process_type   文件下载成功后原文件的处理方式：1-不做处理；2-删除文件；3-移动到备份目录。\n");
    printf("server_path         服务端文件存放的根目录。\n");
    printf("server_path_bak     文件成功下载后，服务端文件备份的根目录，当file_process_type==3时有效。\n");
    printf("and_child           是否下载server_path目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
    printf("match_rules         待下载文件名的匹配规则，如\"*.TXT,*.XML\"\n");
    printf("client_path         客户端文件存放的根目录。\n");
    printf("client_path_bak     文件成功上传后，客户端文件备份的根目录，当file_process_type==3时有效。\n");
    printf("time_tvl            扫描服务目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
    printf("time_out            本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
    printf("process_name        进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

/**
 * @brief 将xml字符串转为参数
 * @param xml_buffer xml字符串
*/
bool _xmlToArg(char *xml_buffer)
{
    memset(&arg, 0, sizeof(arg));

    get_xml_buffer(xml_buffer, "ip", arg.ip);
    if(strlen(arg.ip) == 0)
    {
        logfile.write("ip is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "port", arg.port);
    if(arg.port == 0)
    {
        logfile.write("port is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "client_type", arg.client_type);
    if(arg.client_type != 1 && arg.client_type != 2)
    {
        logfile.write("client_type is %s, but mast be 1 or 2.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "file_process_type", arg.file_process_type);
    if(arg.file_process_type != 1 && arg.file_process_type != 2 && arg.file_process_type != 3)
    {
        logfile.write("file_process_type must in (1, 2, 3).\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "server_path", arg.server_path);
    if(strlen(arg.server_path) == 0)
    {
        logfile.write("server_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "server_path_bak", arg.server_path_bak);
    if(strlen(arg.server_path_bak) == 0 && arg.client_type == 2)
    {
        logfile.write("server_path_bak is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "client_path_bak", arg.client_path_bak);
    if(strlen(arg.client_path_bak) == 0 && arg.client_type == 1)
    {
        logfile.write("client_path_bak is null.\n");
        return false;
    }

    if(get_xml_buffer(xml_buffer, "and_child", arg.server_path_bak) == false)
    {
        logfile.write("You need set prama \"and_chikd\", true or false indicates whether to process files under subdirectories\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "match_rules", arg.match_rules);
    if(strlen(arg.match_rules) == 0)
    {
        logfile.write("match_rules is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "client_path", arg.client_path);
    if(strlen(arg.client_path) == 0)
    {
        logfile.write("client_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "time_tvl", arg.time_tvl);
    if(arg.time_tvl == 0)
    {
        logfile.write("time_tvl is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "time_out", arg.time_out);
    if(arg.time_out == 0)
    {
        logfile.write("time_out is null.\n");
        return false;
    }

    if(arg.time_out <= arg.time_tvl)
    {
        logfile.write("time_out(%d) can't less time_tvl(%d).\n", arg.time_out, arg.time_tvl);
        return false;
    }

    get_xml_buffer(xml_buffer, "process_name", arg.process_name, 50);
    if(strlen(arg.process_name) == 0)
    {
        logfile.write("process_name is null.\n");
        return false;
    }

    return true;
}

/**
 * @brief 登录tcp服务器
 * @param argv 登录参数
*/
bool login(const char *argv)
{
    g_str_send_buffer = argv;

    // 向服务端发送请求报文
    if(tcp_client.write(g_str_send_buffer) == false)
    {
        logfile.write("tcp_client.write(): send login message failed.\n");
        return false;
    }

    // 接受服务端的回应报文
    if(tcp_client.read(g_str_recv_buffer, 10) == false)
    {
        logfile.write("tcp_client.read(): receive login message failed.\n");
        return false;
    }
    
    if(g_str_recv_buffer == "success")
        logfile.write("Login tcp server(%s:%d) success.\n", arg.ip, arg.port);
    else
    {
        logfile.write("Login tcp server(%s:%d) failed: %s.\n", arg.ip, arg.port, g_str_recv_buffer);
        return false;
    }
    return true;
}

/**
 * @brief tcp下载文件的主函数
*/
void _tcpGetFiles()
{
    while(true)
    {
        pactive.upt_atime();

        // 接受服务器报文
        if(tcp_client.read(g_str_recv_buffer, arg.time_tvl + 10) == false)
        {
            logfile.write("tcp_client.read(): recv server message failed.\n");
            return;
        }

        // 处理心跳报文
        if(g_str_recv_buffer == "<activate_test>ok</activate_test>")
        {
            g_str_send_buffer = "ok";

            if(tcp_client.write(g_str_send_buffer) == false)
            {
                logfile.write("tcp_client.write(): processing pactive message failed.\n");
                return;
            }
        }

        // 处理下载文件的请求报文
        if(g_str_recv_buffer.find("<file_name>") != string::npos)
        {
            string server_file_name;
            string server_file_mtime;
            int server_file_size;

            get_xml_buffer(g_str_recv_buffer, "file_name", server_file_name);
            get_xml_buffer(g_str_recv_buffer, "file_mtime", server_file_mtime);
            get_xml_buffer(g_str_recv_buffer, "file_size", server_file_size);

            // 获取客户端文件地址
            string client_file_name = server_file_name;
            replace_str(client_file_name, arg.server_path, arg.client_path, false);

            // 接受文件内容
            logfile.write("recv file(%s) from (%s) ...... ", client_file_name.c_str(), server_file_name.c_str());
            if(recvFile(client_file_name, server_file_mtime, server_file_size) == true)
            {
                logfile << "success.\n";
                s_format(g_str_send_buffer, "<file_name>%s</file_name><result>success</result>", server_file_name.c_str());
            }
            else
            {
                logfile << "failed.\n";
                s_format(g_str_send_buffer, "<file_name>%s</file_name><result>failed</result>", server_file_name.c_str());
            }

            // 返回文件下载结果
            if(tcp_client.write(g_str_send_buffer) == false)
            {
                logfile << "tcp_client.write(): return the result of file download failed.\n";
                return;
            }

        }
    }
}

/**
 * @brief 接受文件的内容
 * @param file_name 文件名
 * @param mtime 文件的修改时间
 * @param file_size 文件大小
*/
bool recvFile(const string &file_name, const string &mtime, int file_size)
{
    int total_bytes = 0;
    int on_read = 0;
    char buffer[1000];
    cofile ofile;

    new_dir(file_name);

    if(ofile.open_file(file_name, true, ios::out | ios::binary) == false)
    {
        logfile.write("ofile.open_file(%s) failed.\n", file_name.c_str());
        return false;
    }

    while(true)
    {
        memset(buffer, 0, sizeof(buffer));

        if(file_size - total_bytes > sizeof(buffer))
            on_read = sizeof(buffer);
        else
            on_read = file_size - total_bytes;

        if(tcp_client.read(buffer, on_read) == false)
        {
            logfile.write("tcp_client.read():downloading file failed.\n");
            return false;
        }

        ofile.write(buffer, on_read);

        total_bytes += on_read;

        if (total_bytes == file_size)
            break;
    }

    ofile.close_and_rename();

    set_mtime(file_name, mtime);

    return true;
}

/**
 * @brief 上传文件
 * @param bcontinue 上传过程中是否有新文件生成（即是否还需上传）
*/
bool _tcpPutFiles(bool &bcontinue)
{
    bcontinue = false;

    cdir dir;

    if(dir.open_dir(arg.client_path, arg.match_rules, 10000, arg.and_child) == false)
    {
        logfile.write("dir.open_dir(%s, %s, 10000, %s) failed.\n", arg.client_path, arg.match_rules, arg.and_child ? "true" : "false");
        return false;
    }

    // 上传文件后，未收到确认报文的文件数量
    int delayed = 0;

    while(dir.read_dir())
    {
        bcontinue = true;

        s_format(g_str_send_buffer, "<file_name>%s</file_name><file_mtime>%s</file_mtime><file_size>%d</file_size>",
                                    dir.m_ffilename.c_str(), dir.m_mtime.c_str(), dir.m_filesize);
        if(tcp_client.write(g_str_send_buffer) == false)
        {
            logfile.write("send file(%s) to server failed.\n", dir.m_ffilename.c_str());
            return false;
        }

        logfile.write("send file(%s) ...... ", dir.m_ffilename.c_str());
        if(sendFile(dir.m_ffilename, dir.m_filesize) == true)
        {
            logfile << "success.\n";
            delayed++;
        }
        else
        {
            logfile << "failed.\n";
            tcp_client.close_connect();
            return false;
        }

        pactive.upt_atime();

        while(delayed > 0)
        {
            if(tcp_client.read(g_str_recv_buffer, -1) == false)
                break;

            delayed--;
            ackMessage(g_str_recv_buffer);
        }
    }

    while(delayed > 0)
    {
        if(tcp_client.read(g_str_recv_buffer, -1) == false)
            break;

        delayed--;
        ackMessage(g_str_recv_buffer);
    }

        
    bcontinue = dir.read_dir();
    return true;
}

/**
 * @brief 将文件发送给服务端
 * @param file_name 文件名
 * @param file_size 文件大小
*/
bool sendFile(const string &file_name, const int file_size)
{
    int on_read = 0;
    char buffer[1000];
    int total_bytes = 0;
    cifile ifile;

    if(ifile.open_file(file_name, ios::in | ios::binary) == false)
    {
        logfile.write("open file(%s) failed.\n", file_name.c_str());
        return false;
    }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        if(file_size - total_bytes > sizeof(buffer))
            on_read = sizeof(buffer);
        else
            on_read = file_size - total_bytes;

        ifile.read(buffer, on_read);

        if(tcp_client.write(buffer, on_read) == false)
        {
            logfile.write("uploading file(%s) failed.\n", file_name);
            return false;
        }

        total_bytes += on_read;

        if(total_bytes == file_size)
            break;
    }
    return true;
}

/**
 * @brief 处理传输文件的响应报文（删除或转存文件）
 * @param str_recv_buffer 接收到的数据
*/
bool ackMessage(const string &str_recv_buffer)
{
    string file_name;
    string upload_result;

    get_xml_buffer(g_str_recv_buffer, "file_name", file_name);
    get_xml_buffer(g_str_recv_buffer, "result", upload_result);

    if(upload_result != "success")
    {
        logfile.write("server recv file failed, upload result(%s).\n", upload_result.c_str());
        return false;
    }

    if(arg.file_process_type == 2)
    {
        if(remove(file_name.c_str()) != 0)
        {
            logfile.write("remove(%s) failed.\n", file_name.c_str());
            return false;
        }
    }

    if(arg.file_process_type == 3)
    {
        string bak_file_name = file_name;
        replace_str(bak_file_name, arg.client_path, arg.client_path_bak, false);
        if(rename_file(file_name, bak_file_name) == false)
        {
            logfile.write("rename_file(%s, %s) failed.\n", file_name.c_str(), bak_file_name.c_str());
            return false;
        }
    }

    return true;
}

/**
 * @brief 检测服务端是否存活
*/
bool activeTest()
{
    g_str_send_buffer = "<active_test>ok</active_test>";

    if(tcp_client.write(g_str_send_buffer) == false)
    {
        logfile.write("tcp_client.write(%s) failed.\n", g_str_send_buffer.c_str());
        return false;
    }

    if(tcp_client.read(g_str_recv_buffer, 10) == false)
    {
        logfile.write("tcp_client.read():response to active message failed.\n");
        return false;
    }

    return true;
}