/**
 * @brief 将全国气象站点参数文件中的数据导入到oracle数据库中的T_ZHOBTCODE表中
*/
#include "_public.h"
#include "_ooci.h"
using namespace idc;

struct ST_Stcode
{
    char provname[31];  // 省
    char obtid[11];         // 站号
    char cityname[31];   // 站名
    char lat[11];             // 纬度
    char lon[11];            // 经度
    char height[11];       // 海拔高度
} st_code;

clogfile log_file;
Connection conn;
cpactive pactive;
list<ST_Stcode> list_stcode;

void _help();
bool load_stcode_from_ini_file(const string &ini_file);
void EXIT(int sig);

int main(int argc,char *argv[])
{
    if(argc != 5) 
    {
        _help();
        EXIT(-1);
    }

    close_io_and_signal();
    signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

    if(log_file.open(argv[4]) == false)
    {
        printf("open log file(%s) failed\n", argv[4]);
        EXIT(-1);
    }

    pactive.add_p_info(10, "obtcodetodb");

    if(load_stcode_from_ini_file(argv[1]) == false)
    {
        log_file.write("load_stcode_from_ini_file(%s) failed.\n", argv[1]);
        EXIT(-1);
    }
    
    if(conn.connecttodb(argv[2], argv[3]) != 0)
    {
        log_file.write("connect database(%s) failed: %s\n", argv[2], conn.message());
        EXIT(-1);
    }
    log_file.write("connect database(%s) successed.\n", argv[2]);

    SqlStatement stmt_ins(&conn);
    stmt_ins.prepare("INSERT INTO T_ZHOBTCODE\
                        (obtid, cityname, provname, lat, lon, height, height, keyid)\
                        values(:1, :2, :3, :4*100, :5*100, :6*10, SEQ_ZHOBTCODE.nextval)");

    return 0;
}

void _help()
{
    printf("\n");
    printf("Usage: ./obtcodetodb ini_file conn_str db_charset log_file\n");
    printf("Example: \n");
    printf("    ./obtcodetodb ../ini/stcode.ini idc/idcpwd@ORCL \"Simplified Chinese_China.AL32UTF8\" ../log/obtcodetodb.log\n");
    printf("    /root/C++/selfC++Framework/tools/cpp/procctl.cpp 120 ./obtcodetodb ../ini/stcode.ini idc/idcpwd@ORCL \"Simplified Chinese_China.AL32UTF8\" ../log/obtcodetodb.log\n");
    printf("brief: 将全国气象站站点的参数数据保存到T_ZHOBTCODE表中\n");
    printf("params: \n");
    printf("    ini_file: 全国气象站点参数文件名（绝对路径）\n");
    printf("    conn_str: 数据库连接字符串，格式\"username/password@tnsname\"\n");
    printf("    db_charset: 数据库的字符集，默认\"Simplified Chinese_China.AL32UTF8\"");
    printf("    log_file: 日志文件名\n");
    printf("notes:\n");
    printf("    若由procctl启动，设置为每120s运行一次\n");
}

/**
 * @brief 从ini文件中加载气象站站点信息
 * @param ini_file ini文件名（包含路径）
*/
bool load_stcode_from_ini_file(const string &ini_file)
{
    cifile ifile;

    if(ifile.open_file(ini_file) == false)
    {
        log_file.write("ifile.open_file(%s) failed\n", ini_file.c_str());
        return false;
    }

    string line_buffer;
    ccmdstr cmd_str;

    while(true)
    {
        if(ifile.read_line(line_buffer) == false)
            break;

        cmd_str.split_to_cmd(line_buffer, ",");

        if(cmd_str.size() != 6)
            continue;
        
        memset(&st_code, 0, sizeof(ST_Stcode));
        cmd_str.getvalue(0, st_code.provname,30); // 省
        cmd_str.getvalue(1, st_code.obtid,5);    // 站号
        cmd_str.getvalue(2, st_code.cityname,30);  // 站名
        cmd_str.getvalue(3, st_code.lat,10);      // 纬度
        cmd_str.getvalue(4, st_code.lon,10);      // 经度
        cmd_str.getvalue(5, st_code.height,10);   // 海拔高度

        list_stcode.push_back(st_code);
    }

    return true;
}

void EXIT(int sig)
{
    log_file.write("Process exit, signal number: %d\n", sig);

    // 在析构函数中会自动回滚事务并断开与数据库的连接，因此此处可以不写
    // conn.rollback();
    // conn.disconnect();

    exit(0);
}
