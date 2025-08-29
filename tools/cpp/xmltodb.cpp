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
    char bak_xml_path[301];   // xml文件入库后的备份目录。
    char err_xml_path[301];    // 入库失败的xml文件存放的目录。
    int  time_tvl;                    // 本程序运行的时间间隔，本程序常驻内存。
    int  time_out;                   // 本程序运行时的超时时间。
    char p_name[51];            // 本程序运行时的程序名。
}st_arg;

// 数据入库参数结构体
struct ST_XmlToTable
{
    char file_name[101];        // xml文件名的匹配规则，使用逗号分隔
    char table_name[31];        // 待入库的表名
    int upt_mark;               // 更新标志，1-更新，2-不更新
    char exec_sql[301];         // 处理xml文件前的sql处理语句
}st_xml_to_table;

clogfile logfile;
Connection conn;
cpactive pactive;
vector<struct ST_XmlToTable> v_st_xml_to_table; // 数据入库的参数的容器
C_TableColumns tablecolumns;    // 获取表的全部及主键字段
string insert_sql;  // 插入数据的SQL语句
string update_sql;  // 更新数据的SQL语句
vector<string> v_columns_value;     // 存放从xml文件中解析出的字段值
SqlStatement stmt_ins, stmt_upt;

void _help();
bool _xml_to_arg(const char *xml_buffer);
void EXIT(int sig);
bool _xml_to_db();

bool load_xml_to_table();
bool find_xml_to_table(const string &xml_file_name);

void splice_ins_or_upt_sql();

void prepare_sql();
bool exec_sql();
void split_buffer(const string &buffer);
bool xml_to_bak_or_err_dir(const string &full_file_name, const string &srv_path, const string &dst_path);

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

    cifile config_file;
    if(config_file.open_file(argv[2]) == false)
    {
        logfile.write("config_file.open_file(%s) failed.\n", argv[2]);
        return -1;
    }
    char* config_buffer = config_file.read_all();
    if(_xml_to_arg(config_buffer) == false)
    {
        logfile.write("_xml_to_args() failed.\n");
        return -1;
    }
    logfile.write("load config file(%s) success.\n", argv[2]);
    
    pactive.add_p_info(st_arg.time_out, st_arg.p_name);

    _xml_to_db();

    return 0; 
}

/**
 * @brief 帮助信息
*/
void _help()
{
    print_dash_line(60);
    printf("Usage: /root/C++/selfC++Framework/tools/bin/xmltodb log_file_name config_file_name\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/xmltodb /root/C++/selfC++Framework/log/xmltodb.log /root/C++/selfC++Framework/tools/bin/xmltodb_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/xmltodb /root/C++/selfC++Framework/log/xmltodb.log /root/C++/selfC++Framework/tools/bin/xmltodb_config.xml\n");
    printf("brief: 将xml文件入库到Oracle数据库的表中\n");
    printf("Param: \n");
    printf("    log_file_name 日志文件名称（全地址）\n");
    printf("    config_file_name 配置文件名称（全地址），配置内容如下：\n");
    printf("        conn_str 数据库的连接参数\n");
    printf("        charset 数据库的字符集\n");
    printf("        ini_file_name 数据库入库的配置文件（全地址）\n");
    printf("        xml_path 待入库的xml文件所在目录\n");
    printf("        bak_xml_path xml文件备份目录\n");
    printf("        err_file_name xml文件入库失败后的存放目录\n");
    printf("        time_tvl xml文件入库的间隔时间（执行数据入库任务的间隔时间），单位：秒\n");
    printf("        time_out 数据入库程序的超时时间，单位：秒\n");
    printf("        p_name 进程名称\n");
    print_dash_line(60);
}

/**
 * @brief 将xml文件的内容解析到st_arg结构体中
 * @param xml_buffer 从xml文件中读取的内容
*/
bool _xml_to_arg(const char *xml_buffer)
{

    return true;
}

/**
 * @brief 程序退出的信号处理函数
*/
void EXIT(int sig)
{

}

/**
 * @brief 数据入库主函数
*/
bool _xml_to_db()
{

    return true;
}

/**
 * @brief 将数据入库的配置文件（st_arg.ini_file_name）加载到v_st_xml_to_table中
*/
bool load_xml_to_table()
{

    return true;
}

/**
 * @brief 根据xml文件名，在v_st_xml_to_table中查找对应的数据入库配置，存放在st_xml_to_table中
 * @param xml_file_name 所需数据入库的xml文件名
*/
bool find_xml_to_table(const string &xml_file_name)
{

    return true;
}

/**
 * @brief 拼接插入或更新sql语句，绑定输入变量
*/
void splice_ins_or_upt_sql()
{

}

/**
 * @brief 准备插入和更新的sql语句，绑定输入变量
*/
void prepare_sql()
{

}

/**
 * @brief 若处理xml文件前的sql处理语句（st_xml_to_table.exec_sql）不为空，则执行
*/
bool exec_sql()
{

    return true;
}

/**
 * @brief 解析xml内容，存在在已绑定的输入变量v_column_value数组中
 * @param buffer xml内容
*/
void split_buffer(const string &buffer)
{

}

/**
 * @brief 将xml文件移动到备份目录或错误目录中
 * @param full_file_name xml文件文件名
 * @param src_path 源文件地址
 * @param dst_path 目标文件地址
*/
bool xml_to_bak_or_err_dir(const string &full_file_name, const string &srv_path, const string &dst_path)
{

    return true;
}
