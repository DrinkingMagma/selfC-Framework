/**
 * @brief 将远程ftp服务端的文件下载到本地目录中
*/
#include "_public.h"
#include "_ftp.h"

using namespace idc;

// 程序运行参数的结构体
struct st_arg
{
    char host[31];                   // FTP服务器的IP和端口
    int mode;                        // 传输模式， 1-被动模式，2-主动模式（缺省为被动模式）
    char username[31];               // FTP服务器登录用户名
    char password[31];               // FTP服务器登录密码
    char remote_path[256];           // FTP服务器上存放文件的目录
    char local_path[256];            // 本地文件存放的目录
    char match_rules[256];           // 待下载的文件的文件名匹配规则
    int remote_file_processing_type; // 下载后服务器上文件的处理方式，1-什么也不做，2-删除，3-备份
    char remote_path_bak[256];       // 下载后服务器上文件备份的目录
    char geted_filename[256];        // 存放已下载成功的文件信息的文件
    bool check_mtime;                // 是否检查文件的最后修改时间，true-检查，false-不检查（缺省为false）
    int out_time;                    // 进程的心跳超时时间
    char process_name[51];           // 进程名，建议采用"ftpgetfiles_后缀"的方式
} starg;

// 文件信息的结构体
struct st_file_info
{
    string filename; // 文件名
    string mtime;    // 文件最后修改时间

    // 启动默认构造函数
    st_file_info() = default;
    // 构造函数
    st_file_info(const string &in_filename, const string &in_mtime) : filename(in_filename), mtime(in_mtime) {};
    // 清空文件名和文件最后修改时间
    void clear()
    {
        filename.clear();
        mtime.clear();
    }
};

clogfile logfile; // 日志文件对象
cftpclient ftp;   // ftp客户端对象
cpactive pactive; // 进程心跳对象

map<string, string> m_downloaded_files;            // 存放已下载成功的文件
list<struct st_file_info> lst_remote_files; // 存放所有远程文件信息
list<struct st_file_info> lst_excluded_files;      // 存放本次不需要下载的文件
list<struct st_file_info> lst_download_files;      // 存放本次需要下载的文件

void _help();
void EXIT(int sig);
bool _xmltoarg(const char *str_xml_buffer);
bool load_downloaded_files();
bool load_list_files();
bool get_excluded_and_download_files();
bool write_excluded_files();
bool append_downloaded_files(struct st_file_info &stfile_info);

int main(int argc, char *argv[])
{
    if (argc != 3)
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

    cifile file;
    if(file.open_file(argv[2]) == false)
    {
        logfile.write("file.open_file(%s) failed.\n", argv[2]);
        return -1;
    }

    char *str_xml = file.read_all();
    if(_xmltoarg(str_xml) == false)
    {
        logfile.write("_xmltoarg() failed.\n");
        return -1;
    }

    pactive.add_p_info(starg.out_time, starg.process_name);

    if(ftp.login(starg.host, starg.username, starg.password, starg.mode) == false)
    {
        logfile.write("ftp.login(%s, %s, %s, %d) failed.\n", starg.host, starg.username, starg.password, starg.mode);
        return -1;
    }
    logfile.write("ftp.login(%s, %s, %s, %d) ok.\n", starg.host, starg.username, starg.password, starg.mode);

    if(ftp.chdir(starg.remote_path) == false)
    {
        logfile.write("ftp.chdir(%s) failed: %s\n", starg.remote_path, ftp.response());
        return -1;
    }

    if(ftp.nlist(".", s_format("/tmp/nlist/ftpgetfiles_%d.nlist", getpid())) == false)
    {
        logfile.write("ftp.nlist(%s) failed: %s\n", starg.remote_path, ftp.response());
        return -1;
    }

    pactive.upt_atime();

    if(load_list_files() == false)
    {
        logfile.write("load_list_files() failed.\n");
        return -1;
    }

    if(starg.remote_file_processing_type == 1)
    {
        load_downloaded_files();

        get_excluded_and_download_files();

        write_excluded_files();
    }
    else
    {
        // 为了统一文件下载的代码，把容器二和容器四交换。
        lst_remote_files.swap(lst_download_files);
    }

    pactive.upt_atime();

    string str_remote_filename;
    string str_local_filename;

    for(auto &aa : lst_download_files)
    {
        s_format(str_remote_filename, "%s/%s", starg.remote_path, aa.filename.c_str());
        s_format(str_local_filename, "%s/%s", starg.local_path, aa.filename.c_str());

        logfile.write("get file %s to %s ...... ", str_remote_filename.c_str(), str_local_filename.c_str());

        if(ftp.get(str_remote_filename, str_local_filename, starg.check_mtime) == false)
        {
            logfile << "failed: " << ftp.response() << "\n";
            return -1;
        }
        logfile << "finished.\n";

        pactive.upt_atime();

        // 将下载成功的文件添追加到已下载文件列表中
        if(starg.remote_file_processing_type == 1)
            append_downloaded_files(aa);

        // 将服务端的文件删除
        else if(starg.remote_file_processing_type == 2)
        {
            if(ftp.ftpdelete(str_remote_filename) == false)
            {
                logfile.write("ftp.ftpdelete(%s) failed: %s\n", str_remote_filename, ftp.response());
                return -1;
            }
        }

        // 将服务端的文件移动到备份目录
        else if(starg.remote_file_processing_type == 3)
        {
            string str_remote_filename_bak = s_format("%s/%s", starg.remote_path_bak, aa.filename);
            if(ftp.ftprename(str_remote_filename, str_remote_filename_bak) == false)
            {
                logfile.write("ftp.ftprename(%s, %s) failed: %s\n", str_remote_filename, str_remote_filename_bak, ftp.response());
                return -1;
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
    printf("Using: /root/C++/selfC++Framework/tools/bin/ftpgetfiles log_filename xml_filename\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/ftpgetfiles /root/C++/selfC++Framework/log/ftpgetfiles.log /root/C++/selfC++Framework/tools/cpp/ftpgetfiles_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 30 /root/C++/selfC++Framework/tools/bin/ftpgetfiles /root/C++/selfC++Framework/log/ftpgetfiles.log /root/C++/selfC++Framework/tools/cpp/ftpgetfiles_config.xml\n");
    printf("功能：将远程ftp服务端的文件下载到本地目录\n");
    printf("参数说明：\n"
           "log_filename: 日志文件名\n"
           "xml_filename: 参数配置文件，具体参数如下：\n");
    printf("host: 远程服务端的IP和端口。\n");
    printf("mode: 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("username: 远程服务端ftp的用户名。\n");
    printf("password: 远程服务端ftp的密码。\n");
    printf("remote_path: 远程服务端存放文件的目录。\n");
    printf("localpath: 本地文件存放的目录。\n");
    printf("matchrules: 待下载文件匹配的规则。"
           "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("ptype: 文件下载成功后，远程服务端文件的处理方式："
           "1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("remote_pathbak: 文件下载成功后，服务端文件的备份目录，"
           "此参数只有当ptype=3时才有效。\n");
    printf("okfilename: 已下载成功文件名清单，"
           "此参数只有当ptype=1时才有效。\n");
    printf("checkmtime: 是否需要检查服务端文件的时间，true-需要，false-不需要，"
           "此参数只有当ptype=1时才有效，缺省为false。\n");
    printf("timeout: 下载文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("pname: 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}

/**
 * @brief 程序退出和信号2、15处理函数
 */
void EXIT(int sig)
{
    printf("Program exit, sig = %d\n", sig);

    exit(0);
}

/**
 * @brief 将xml解析到st_arg结构体中
 * @param str_xml_buffer xml字符串
 */
bool _xmltoarg(const char *str_xml_buffer)
{
    memset(&starg, 0, sizeof(struct st_arg));

    get_xml_buffer(str_xml_buffer, "host", starg.host, 30);
    if (strlen(starg.host) == 0)
    {
        logfile.write("host is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "mode", starg.mode); // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    if (starg.mode != 2)
        starg.mode = 1;

    get_xml_buffer(str_xml_buffer, "username", starg.username, 30); // 远程服务端ftp的用户名。
    if (strlen(starg.username) == 0)
    {
        logfile.write("username is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "password", starg.password, 30); // 远程服务端ftp的密码。
    if (strlen(starg.password) == 0)
    {
        logfile.write("password is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "remotepath", starg.remote_path, 255); // 远程服务端存放文件的目录。
    if (strlen(starg.remote_path) == 0)
    {
        logfile.write("remotepath is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "localpath", starg.local_path, 255); // 本地文件存放的目录。
    if (strlen(starg.local_path) == 0)
    {
        logfile.write("local_path is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "matchrules", starg.match_rules, 100); // 待下载文件匹配的规则。
    if (strlen(starg.match_rules) == 0)
    {
        logfile.write("match_rules is null.\n");
        return false;
    }

    // 下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份。
    get_xml_buffer(str_xml_buffer, "ptype", starg.remote_file_processing_type);
    if ((starg.remote_file_processing_type != 1) && (starg.remote_file_processing_type != 2) && (starg.remote_file_processing_type != 3))
    {
        logfile.write("remote_file_processing_type is error.\n");
        return false;
    }

    // 下载后服务端文件的备份目录。
    if (starg.remote_file_processing_type == 3)
    {
        get_xml_buffer(str_xml_buffer, "remote_pathbak", starg.remote_path_bak, 255);
        if (strlen(starg.remote_path_bak) == 0)
        {
            logfile.write("remote_path_bak is null.\n");
            return false;
        }
    }

    if(starg.remote_file_processing_type == 1)
    {
        get_xml_buffer(str_xml_buffer, "okfilename", starg.geted_filename, 255);
        if(strlen(starg.geted_filename) == 0)
        {
            logfile.write("okfilename is null.\n");
            return false;
        }

        get_xml_buffer(str_xml_buffer, "checkmtime", starg.check_mtime);
    }

    get_xml_buffer(str_xml_buffer, "timeout", starg.out_time);
    if(starg.out_time == 0)
    {
        logfile.write("timeout is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "pname", starg.process_name, 50);

    return true;
}

/**
 * @brief 将starg.geted_filename文件中的内容存放到m_downloaded_files中
*/
bool load_downloaded_files()
{
    if(starg.remote_file_processing_type != 1)
        return true;

    m_downloaded_files.clear();

    cifile file;

    // 若程序第一次运行，starg.geted_filename文件不存在，因此返回true
    if(file.open_file(starg.geted_filename) == false)
        return true;

    string str_buffer;
    st_file_info stfile_info;

    while(true)
    {
        stfile_info.clear();

        if(file.read_line(str_buffer) == false)
            break;
        
        get_xml_buffer(str_buffer, "filename", stfile_info.filename);
        get_xml_buffer(str_buffer, "mtime", stfile_info.mtime);

        m_downloaded_files[stfile_info.filename] = stfile_info.mtime;
    }

    return true;
}


/**
 * @brief 将ftp.nlist()获取的列表文件保存到m_list_files中
*/
bool load_list_files()
{
    lst_remote_files.clear();

    cifile file;
    if(file.open_file(s_format("/tmp/nlist/ftpgetfiles_%d.nlist", getpid())) == false)
    {
        logfile.write("file.open(%s) failed.\n", s_format("/tmp/nlist/ftpgetfiles_%d.nlist", getpid()));
        return false;
    }

    string str_filename;

    while(true)
    {
        if(file.read_line(str_filename) == false)
            break;

        if(match_str(str_filename, starg.match_rules) == false)
            continue;

        if(starg.remote_file_processing_type == 1 && starg.check_mtime == true)
        {
            if(ftp.mtime(str_filename) == false)
            {
                logfile.write("ftp.mtime(%s) failed.\n", str_filename.c_str());
                return false;
            }
        }

        lst_remote_files.emplace_back(str_filename, ftp.m_mtime);
    }

    file.close_and_remove();

    return true;
}

/**
 * @brief 通过对比m_geted_files和lst_remote_files， 获取lst_download_filse和lst_excluded_files
*/
bool get_excluded_and_download_files()
{
    lst_download_files.clear();
    lst_excluded_files.clear();

    for(auto &aa : lst_remote_files)
    {
        auto it = m_downloaded_files.find(aa.filename);
        
        // logfile.write("aa.filename = %s, aa.mtime = %s\n", aa.filename.c_str(), aa.mtime.c_str());
        // logfile.write("it->second = %s\n", it->second.c_str());
        if(it != m_downloaded_files.end())
        {
            if(starg.check_mtime == true)
            {
                if(it->second == aa.mtime)
                    lst_excluded_files.push_back(aa);
                else
                    lst_download_files.push_back(aa);
            }
            else
                lst_excluded_files.push_back(aa);
        }
        else
            lst_download_files.push_back(aa);
    }

    if(lst_download_files.size() == 0)
        logfile.write("no file need to download\n");
    else
        logfile.write("There are %d files need to download.\n", lst_download_files.size());

    return true;
}


/**
 * @brief 将lst_excluded_files写入starg.geted_filename中，覆盖旧文件
*/
bool write_excluded_files()
{
    cofile file;

    if(file.open_file(starg.geted_filename) == false)
    {
        logfile.write("open %s error.\n", starg.geted_filename);
        return false;
    }

    for(auto &aa : lst_excluded_files)
        file.write_line("<filename>%s</filename><mtime>%s</mtime>\n", aa.filename.c_str(), aa.mtime.c_str());

    file.close_and_rename();

    return true;
}


/**
 * @brief 将下载成功的文件追加到starg.geted_filename中
 * @param stfile_info 存放当前下载文件文件信息的结构体
*/
bool append_downloaded_files(struct st_file_info &stfile_info)
{
    cofile file;

    if(file.open_file(starg.geted_filename, false, ios::app) == false)
    {
        logfile.write("open %s failed.\n", starg.geted_filename);
        return false;
    }

    file.write_line("<filename>%s</filename><mtime>%s</mtime>\n", stfile_info.filename.c_str(), stfile_info.mtime.c_str());

    return true;
}
