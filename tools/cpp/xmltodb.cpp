/**
 * @brief 将xml文件中的数据插入数据库中
*/

#include "_tools.h"
using namespace idc;

// 程序运行参数结构体
struct ST_Arg
{
    char conn_str[101];          // 数据库的连接参数。
    char charset[51];            // 数据库的字符集。
    char ini_file_name[301];    // 数据入库的参数配置文件。
    char xml_path[301];         // 待入库xml文件存放的目录。
    char xml_path_bak[301];   // xml文件入库后的备份目录。
    char err_xml_path[301];    // 入库失败的xml文件存放的目录。
    int  time_tvl;                    // 本程序运行的时间间隔，本程序常驻内存。
    int  time_out;                   // 本程序运行时的超时时间。
    char p_name[51];            // 本程序运行时的程序名。
}st_arg;

// 数据入库参数结构体
struct ST_XmlToTable
{
    char file_name[101];
    char table_name[31];
    int upt_
}st_xml_to_table;

clogfile logfile;
Connection conn;

void _help();
bool _xml_to_arg(const char *xml_buffer);
void EXIT();
bool _xml_to_db();

int main(int argc, char *argv[])
{
    print_line(60, '-');
    print_line(60, '^');

    C_TableColumns tc;

    return 0; 
}