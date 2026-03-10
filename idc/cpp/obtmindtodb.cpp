/**
 * @brief 读取全国气象观测数据文件并插入到T_ZHOBTMIND表中，支持xml和csv格式，数据只进行插入，不进行更新
 */

#include "idcapp.h"
using namespace idc;

clogfile logfile;
Connection conn;
cpactive pactive;

void _help();
void EXIT(int sig);
bool _obtmindtodb(const char *path_name, const char *conn_str, const char *charset);



int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        _help();
        return -1;
    }

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if(logfile.open(argv[4]) == false)
    {
        printf("logfile.open(%s) failed.\n", argv[4]);
        return -1;
    }

    pactive.add_p_info(30, "obtmindtodb");

    _obtmindtodb(argv[1], argv[2], argv[3]);

    return 0; 
}

void _help()
{
    print_dash_line(60);
    printf("Usage: ./obtmindtodb path_name conn_str charset logfile\n");
    printf("Example 1: /root/C++/selfC++Framework/idc/bin/obtmindtodb /root/C++/selfC++Framework/idc/output/surfdata "\
            "\"idc/idcpwd@ORCL\" \"Simplified Chinese_China.AL32UTF8\" /root/C++/selfC++Framework/idc/log/obtmindtodb.log\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/idc/bin/obtmindtodb /root/C++/selfC++Framework/idc/output/surfdata "\
            "\"idc/idcpwd@ORCL\" \"Simplified Chinese_China.AL32UTF8\" /root/C++/selfC++Framework/idc/log/obtmindtodb.log\n");
    printf("brief: 读取全国气象观测数据文件并插入到T_ZHOBTMIND表中，支持xml和csv格式，数据只进行插入，不进行更新\n");
    printf("path_name: 数据文件存放的目录\n");
    printf("conn_str: 数据库连接字符串，格式为：username/password@tnsname\n");
    printf("charset: 数据库的字符集，默认为：Simplified Chinese_China.AL32UTF8\n");
    printf("log_file: 日志文件名\n");
    print_dash_line(60);
}

void EXIT(int sig)
{
    logfile.write("Program exit, sig: %d\n", sig);

    exit(0);
}

bool _obtmindtodb(const char *path_name, const char *conn_str, const char *charset)
{
    cdir dir;
    string match_rulers = "*.xml,*.csv";
    if(dir.open_dir(path_name, match_rulers) == false)
    {
        logfile.write("dir.open_dir(%s, %s) failed.\n", path_name, match_rulers);
        return false;
    }

    C_ZHOBTMIND zhobtmind(conn, logfile);

    while(true)
    {
        if(dir.read_dir() == false)
            break;

        if(conn.isopen() == false)
        {
            if(conn.connecttodb(conn_str, charset) != 0)
            {
                logfile.write("connect database(%s) failed.\n%s\n", conn_str, conn.message());
                return false;
            }
            logfile.write("connect database(%s) ok.\n", conn_str);
        }

        cifile ifile;
        if(ifile.open_file(dir.m_ffilename) == false)
        {
            logfile.write("ifile.open_file(%s) failed.\n", dir.m_ffilename.c_str());
            return false;
        }

        int total_count = 0;
        int inserted_count = 0;
        ctimer timer;   // 计时器
        bool b_is_xml = match_str(dir.m_ffilename, "*.xml");
        string str_buffer;

        // 若为csv文件，排除第一行（标题行）
        if(b_is_xml == false)
            ifile.read_line(str_buffer);

        while (true)
        {
            if(b_is_xml == true)
            {
                if(ifile.read_line(str_buffer, "<endl/>") == false)
                {
                    break;
                }
            }
            else
            {
                if(ifile.read_line(str_buffer) == false)
                    break;
            }
            total_count++;

            zhobtmind.split_buffer(str_buffer, b_is_xml);

            if(zhobtmind.insert_to_t_zhobtmind() == true)
                ++inserted_count;
        }

        ifile.close_and_remove();
        conn.commit();
        logfile.write("process file: %s, total_count: %d, inserted_count: %d, took %.2f s\n", dir.m_ffilename.c_str(), total_count, inserted_count, timer.elapsed());
        pactive.upt_atime();
    }

    return true;
}