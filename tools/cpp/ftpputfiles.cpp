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
    char match_rules[256];           // 待上传的文件的文件名匹配规则
    int remote_file_processing_type; // 上传后客户端上文件的处理方式，1-什么也不做，2-删除，3-备份
    char local_path_bak[256];        // 上传后客户端上文件备份的目录
    char puted_filename[256];        // 存放已上传成功的文件信息的文件
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

map<string, string> m_uploaded_files;         // 存放已上传成功的文件
list<struct st_file_info> lst_local_files;    // 存放客户端文件信息
list<struct st_file_info> lst_excluded_files; // 存放本次不需要上传的文件
list<struct st_file_info> lst_upload_files;   // 存放本次需要上传的文件

void _help();
void EXIT(int sig);
bool _xmltoarg(const char *str_xml_buffer);
bool load_local_files();
bool load_uploaded_files();
bool get_excluded_and_upload_files();
bool write_excluded_files();
bool append_upload_files(struct st_file_info &stfile_info);

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
        logfile.write("ftp.login(%s, %s, %s, %d) failed: %s\n", starg.host, starg.username, starg.password, starg.mode, ftp.response());
        return -1;
    }
    logfile.write("ftp.login(%s, %s, %s, %d) ok.\n", starg.host, starg.username, starg.password, starg.mode);

    if(load_local_files() == false)
    {
        logfile.write("load_local_files() failed.\n");
        return -1;
    }

    pactive.upt_atime();

    if(starg.remote_file_processing_type == 1)
    {
        load_uploaded_files();

        get_excluded_and_upload_files();

        write_excluded_files();
    }
    else
        lst_local_files.swap(lst_upload_files);

    pactive.upt_atime();

    string strremote_filename;
    string strlocal_filename;

    for(auto &aa : lst_upload_files)
    {
        s_format(strremote_filename, "%s/%s", starg.remote_path, aa.filename.c_str());
        s_format(strlocal_filename, "%s/%s", starg.local_path, aa.filename.c_str());

        logfile.write("put %s -----> %s ...... ", strlocal_filename.c_str(), strremote_filename.c_str());

        if(ftp.put(strlocal_filename, strremote_filename) == false)
        {
            logfile << "failed: " << ftp.response() << "\n";
            return -1;
        }
        logfile << "finished.\n" ;

        pactive.upt_atime();

        if(starg.remote_file_processing_type == 1)
            append_upload_files(aa);
        else if(starg.remote_file_processing_type == 2)
        {
            if(remove(strlocal_filename.c_str()) != 0)
            {
                logfile.write("remove(%s) failed.\n", strlocal_filename.c_str());
                return -1;
            }
        }
        else if(starg.remote_file_processing_type == 3)
        {
            string strlocal_filename_bak = s_format("%s/%s", starg.local_path_bak, strlocal_filename.c_str());
            if(rename_file(strlocal_filename, strlocal_filename_bak) == false)
            {
                logfile.write("rename_file(%s, %s) failed.\n", strlocal_filename.c_str(), strlocal_filename_bak.c_str());
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
    printf("Using: /root/C++/selfC++Framework/tools/bin/ftpgetfiles log_filename filename\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/ftpputfiles /root/C++/selfC++Framework/log/ftpputfiles.log /root/C++/selfC++Framework/tools/cpp/ftpgetfiles_config.xml\n");
    printf("Example 2: /root/C++/selfC++Frame/tools/bin/procctl/ 30 /root/C++/selfC++Framework/tools/bin/ftpputfiles /root/C++/selfC++Framework/log/ftpputfiles.log /root/C++/selfC++Framework/tools/cpp/ftpgetfiles_config.xml\n");
    printf("功能：将本地目录中的文件上传到服务器的远程目录中。\n");
    printf("参数说明：\n"
           "log_filename: 日志文件名\n"
           "filename: 文件的下载参数， 具体如下：\n");
    printf("host: 远程服务端的IP和端口。\n");
    printf("mode: 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("username: 远程服务端ftp的用户名。\n");
    printf("password: 远程服务端ftp的密码。\n");
    printf("remotepath: 远程服务端存放文件的目录。\n");
    printf("localpath: 本地文件存放的目录。\n");
    printf("matchrules: 待上传文件匹配的规则。"
           "不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("ptype: 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("localpathbak: 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("okfilename: 已上传成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("timeout: 上传文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("pname: 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n");
}

/**
 * @brief 程序退出和信号2、15处理函数
 */
void EXIT(int sig)
{
    printf("program exit, sig = %d\n", sig);

    exit(0);
}

/**
 * @brief 将xml解析到st_arg结构体中
 * @param str_xml_buffer xml字符串
 */
bool _xmltoarg(const char *str_xml_buffer)
{
    memset(&starg, 0, sizeof(struct st_arg));

    get_xml_buffer(str_xml_buffer, "host", starg.host, 30); // 远程服务端的IP和端口。
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
        logfile.write("localpath is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "matchrules", starg.match_rules, 100); // 待上传文件匹配的规则。
    if (strlen(starg.match_rules) == 0)
    {
        logfile.write("matchrules is null.\n");
        return false;
    }

    // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
    get_xml_buffer(str_xml_buffer, "ptype", starg.remote_file_processing_type);
    if ((starg.remote_file_processing_type != 1) && (starg.remote_file_processing_type != 2) && (starg.remote_file_processing_type != 3))
    {
        logfile.write("ptype is error.\n");
        return false;
    }

    if (starg.remote_file_processing_type == 3)
    {
        get_xml_buffer(str_xml_buffer, "localpathbak", starg.local_path_bak, 255); // 上传后客户端文件的备份目录。
        if (strlen(starg.local_path_bak) == 0)
        {
            logfile.write("localpathbak is null.\n");
            return false;
        }
    }

    if (starg.remote_file_processing_type == 1)
    {
        get_xml_buffer(str_xml_buffer, "okfilename", starg.puted_filename, 255); // 已上传成功文件名清单。
        if (strlen(starg.puted_filename) == 0)
        {
            logfile.write("okfilename is null.\n");
            return false;
        }
    }

    get_xml_buffer(str_xml_buffer, "timeout", starg.out_time); // 进程心跳的超时时间。
    if (starg.out_time == 0)
    {
        logfile.write("timeout is null.\n");
        return false;
    }

    get_xml_buffer(str_xml_buffer, "pname", starg.process_name, 50); // 进程名。
    // if (strlen(starg.pname)==0) { logfile.write("pname is null.\n");  return false; }

    return true;
}
/**
 * @brief 将starg.local_path中的文件列表加载到lst_local_files
 */
bool load_local_files()
{
    lst_local_files.clear();

    cdir dir;

    if(dir.open_dir(starg.local_path, starg.match_rules) == false)
    {
        logfile.write("dir.open_dir(%s, %s) failed.\n", starg.local_path, starg.match_rules);
        return false;
    }
    while(true)
    {
        if(dir.read_dir() == false)
            break;

        lst_local_files.emplace_back(dir.m_filename, dir.m_mtime);
        // logfile.write("local_file: %s, mtime: %s\n", dir.m_filename.c_str(), dir.m_mtime.c_str());
    }

    if(lst_local_files.empty())
    {
        logfile.write("No files in %s with the rule of %s.\n", starg.local_path, starg.match_rules);
        return false;
    }

    return true;
}

/**
 * @brief 将starg.puted_filename中的文件列表加载到m_uploaded_files
 */
bool load_uploaded_files()
{
    m_uploaded_files.clear();

    cifile file;

    if(file.open_file(starg.puted_filename) == false)
        return true;

    string strbuffer;

    struct st_file_info stfile_info;

    while(true)
    {
        stfile_info.clear();

        if(file.read_line(strbuffer) == false)
            break;

        get_xml_buffer(strbuffer, "filename", stfile_info.filename);
        get_xml_buffer(strbuffer, "mtime", stfile_info.mtime);

        m_uploaded_files[stfile_info.filename] = stfile_info.mtime;
    }

    return true;
}

/**
 * @brief 通过对比m_puted_files和lst_local_files， 获取lst_upload_filse和lst_excluded_files
 */
bool get_excluded_and_upload_files()
{
    lst_upload_files.clear();
    lst_excluded_files.clear();

    for(auto &aa : lst_local_files)
    {
        auto it = m_uploaded_files.find(aa.filename);

        // logfile.write("aa.filename = %s, aa.mtime = %s\n", aa.filename.c_str(), aa.mtime.c_str());
        // logfile.write("it->second = %s\n", it->second.c_str());

        if(it != m_uploaded_files.end())
        {
            if(it->second == aa.mtime)
                lst_excluded_files.push_back(aa);
            else
                lst_upload_files.push_back(aa);
        }
        else
        {
            lst_upload_files.push_back(aa);
        }
    }

    if(lst_upload_files.size() == 0)
        logfile.write("no file need to upload\n");
    else
        logfile.write("There are %d files need to upload.\n", lst_upload_files.size());

    return true;
}

/**
 * @brief 将lst_excluded_files写入starg.puted_filename中，覆盖旧文件
 */
bool write_excluded_files()
{
    cofile file;

    if(file.open_file(starg.puted_filename) == false)
    {
        logfile.write("file.open_file(%s) failed.", starg.puted_filename);
        return false;
    }

    for(auto &aa : lst_excluded_files)
        file.write_line("<filename>%s</filename><mtime>%s</mtime>\n", aa.filename.c_str(), aa.mtime.c_str());

    file.close_and_rename();

    return true;
}

/**
 * @brief 将上传成功的文件追加到starg.puted_filename中
 * @param stfile_info 存放当前上传文件文件信息的结构体
 */
bool append_upload_files(struct st_file_info &stfile_info)
{
    cofile file;

    if(file.open_file(starg.puted_filename, false, ios::app) == false)
    {
        logfile.write("file.open_file(%s) failed.\n", starg.puted_filename);
        return false;
    }

    file.write_line("<filename>%s</filename><mtime>%s</mtime>\n", stfile_info.filename, stfile_info.mtime);

    return true;
}
