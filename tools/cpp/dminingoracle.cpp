/**
 * @brief 从oracle数据库中获取指定表内数据并写入xml文件(数据抽取)
 */

#include "_public.h"
#include "_ooci.h"
using namespace idc;

// 程序运行参数结构体
struct ST_Arg
{
    char conn_str[101];       // 数据库的连接参数。
    char charset[51];         // 数据库的字符集。
    char select_sql[1024];    // 从数据源数据库抽取数据的SQL语句。
    char field_str[501];      // 抽取数据的SQL语句输出结果集字段名，字段名之间用逗号分隔。
    char field_len[501];      // 抽取数据的SQL语句输出结果集字段的长度，用逗号分隔。
    char begin_file_name[31]; // 输出xml文件的前缀。
    char end_file_name[31];   // 输出xml文件的后缀。
    char out_path[256];       // 输出xml文件存放的目录。
    int max_count;            // 输出xml文件最大记录数，0表示无限制。xml文件将用于入库，如果文件太大，数据库会产生大事务。
    char start_time[52];      // 程序运行的时间区间
    char inc_field[31];       // 递增字段名。
    char inc_file_name[256];  // 已抽取数据的递增字段最大值存放的文件。
    char conn_str1[101];      // 已抽取数据的递增字段最大值存放的数据库的连接参数。
    int timeout;              // 进程心跳的超时时间。
    char p_name[51];          // 进程名，建议用"dminingoracle_后缀"的方式。
} st_arg;

ccmdstr field_name; // 结果集的字段名数组
ccmdstr field_len;  // 结果集字段的长度数组
Connection conn;
clogfile logfile;
long max_inc_value;     // 递增字段的最大值
int inc_field_pos = -1; // 递增字段在结果集数组中的位置
cpactive pactive;

void _help();
void EXIT(int sig);
bool _xml_to_arg(const char *xml_buffer);
bool is_in_start_time();
bool _dminingoracle();
bool read_inc_field();
bool write_inc_field();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        _help();
        return -1;
    }

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if (logfile.open(argv[1]) == false)
    {
        printf("logfile.open(%s) failed.\n", argv[1]);
        return -1;
    }

    cifile xml_file;
    if (xml_file.open_file(argv[2]) == false)
    {
        logfile.write("xml_file.open_file(%s) failed.\n", argv[2]);
        return -1;
    }

    char *xml_buffer = xml_file.read_all();

    if (_xml_to_arg(xml_buffer) == false)
    {
        logfile.write("_xml_to_arg() failed.\n");
        EXIT(-1);
    }

    if (is_in_start_time() == false)
    {
        logfile.write("cur time %s is not in start time %s.\n", time_to_str(time(nullptr), ""), st_arg.start_time);
        EXIT(0);
    }

    pactive.add_p_info(st_arg.timeout, st_arg.p_name);

    if (conn.connecttodb(st_arg.conn_str, st_arg.charset) != 0)
    {
        logfile.write("connect database(%s) failed: %s\n", st_arg.conn_str, conn.message());
        EXIT(-1);
    }
    logfile.write("connect database(%s) ok.\n", st_arg.conn_str);

    if (read_inc_field() == false)
    {
        logfile.write("read_inc_field() failed.\n");
        EXIT(-1);
    }

    _dminingoracle();

    return 0;
}

/**
 * @brief 显示程序相关信息
 */
void _help()
{
    print_dash_line(60);
    printf("Usage: /root/C++/selfC++Framework/tools/bin/dminingoracle logfile run_xml_file\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/dminingoracle /root/C++/selfC++Framework/log/dminingoracle.log /root/C++/selfC++Framework/tools/cpp/dminingoracle_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/dminingoracle /root/C++/selfC++Framework/log/dminingoracle.log "
           "/root/C++/selfC++Framework/tools/cpp/dminingoracle_config.xml\n");
    printf("brief: 从oracle中提取数据并保存到xml文件中\n");
    printf("Param: \n");
    printf("    logfile:        日志文件\n");
    printf("    run_xml_file:   包含程序运行参数的xml文件\n");
    printf("        xmlfile params:\n");
    printf("            conn_str        数据源数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("            charset         数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("            select_sql      从数据源数据库抽取数据的SQL语句，如果是增量抽取，一定要用递增字段作为查询条件，如where keyid>:1。\n");
    printf("            field_str       抽取数据的SQL语句输出结果集的字段名列表，中间用逗号分隔，将作为xml文件的字段名。\n");
    printf("            field_len       抽取数据的SQL语句输出结果集字段的长度列表，中间用逗号分隔。field_str与field_len的字段必须一一对应。\n");
    printf("            out_path        输出xml文件存放的目录。\n");
    printf("            begin_file_name 输出xml文件的前缀。\n");
    printf("            end_file_name   输出xml文件的后缀。\n");
    printf("            max_count       输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
    printf("            start_time      程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"
           "如果start_time为空，表示不启用，只要本程序启动，就会执行数据抽取任务，为了减少数据源数据库压力"
           "抽取数据的时候，如果对时效性没有要求，一般在数据源数据库空闲的时候时进行。\n");
    printf("            inc_field       递增字段名，它必须是field_str中的字段名，并且只能是整型，一般为自增字段。"
           "如果inc_field为空，表示不采用增量抽取的方案。");
    printf("            inc_file_name   已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
    printf("            conn_str1       已抽取数据的递增字段最大值存放的数据库的连接参数。conn_str1和inc_file_name二选一，conn_str1优先。\n");
    printf("            timeout         本程序的超时时间，单位：秒。\n");
    printf("            p_name          进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n");
    print_dash_line(60);
}

/**
 * @brief 程序退出函数
 * @param sig 退出信号
 */
void EXIT(int sig)
{
    logfile.write("Program exit, sig = %d\n", sig);
    exit(0);
}

/**
 * @brief 将xml字符串转换成参数
 * @param xml_buffer xml字符串
 */
bool _xml_to_arg(const char *xml_buffer)
{
    memset(&st_arg, 0, sizeof(st_arg));

    get_xml_buffer(xml_buffer, "conn_str", st_arg.conn_str, 100); // 数据源数据库的连接参数。
    if (strlen(st_arg.conn_str) == 0)
    {
        logfile.write("conn_str is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "charset", st_arg.charset, 50); // 数据库的字符集。
    if (strlen(st_arg.charset) == 0)
    {
        logfile.write("charset is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "select_sql", st_arg.select_sql, 1000); // 从数据源数据库抽取数据的SQL语句。
    if (strlen(st_arg.select_sql) == 0)
    {
        logfile.write("select_sql is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "field_str", st_arg.field_str, 500); // 结果集字段名列表。
    if (strlen(st_arg.field_str) == 0)
    {
        logfile.write("field_str is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "field_len", st_arg.field_len, 500); // 结果集字段长度列表。
    if (strlen(st_arg.field_len) == 0)
    {
        logfile.write("field_len is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "begin_file_name", st_arg.begin_file_name, 30); // 输出xml文件存放的目录。
    if (strlen(st_arg.begin_file_name) == 0)
    {
        logfile.write("begin_file_name is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "end_file_name", st_arg.end_file_name, 30); // 输出xml文件的前缀。
    if (strlen(st_arg.end_file_name) == 0)
    {
        logfile.write("end_file_name is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "out_path", st_arg.out_path, 255); // 输出xml文件的后缀。
    if (strlen(st_arg.out_path) == 0)
    {
        logfile.write("out_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "max_count", st_arg.max_count); // 输出xml文件的最大记录数，可选参数。

    get_xml_buffer(xml_buffer, "start_time", st_arg.start_time, 50); // 程序运行的时间区间，可选参数。

    get_xml_buffer(xml_buffer, "inc_field", st_arg.inc_field, 30); // 递增字段名，可选参数。

    get_xml_buffer(xml_buffer, "inc_file_name", st_arg.inc_file_name, 255); // 已抽取数据的递增字段最大值存放的文件，可选参数。

    get_xml_buffer(xml_buffer, "conn_str1", st_arg.conn_str1, 100); // 已抽取数据的递增字段最大值存放的数据库的连接参数，可选参数。

    get_xml_buffer(xml_buffer, "timeout", st_arg.timeout); // 进程心跳的超时时间。
    if (st_arg.timeout == 0)
    {
        logfile.write("timeout is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "p_name", st_arg.p_name, 50); // 进程名。
    if (strlen(st_arg.p_name) == 0)
    {
        logfile.write("p_name is null.\n");
        return false;
    }

    // 拆分starg.fieldstr到fieldname中。
    field_name.split_to_cmd(st_arg.field_str, ",");

    // 拆分starg.fieldlen到fieldlen中。
    field_len.split_to_cmd(st_arg.field_len, ",");

    // 判断fieldname和fieldlen两个数组的大小是否相同。
    if (field_len.size() != field_name.size())
    {
        logfile.write("field_str和field_len的元素个数不一致。\n");
        return false;
    }

    // 如果是增量抽取，incfilename和connstr1必二选一。
    if (strlen(st_arg.inc_field) > 0)
    {
        if ((strlen(st_arg.inc_file_name) == 0) && (strlen(st_arg.conn_str1) == 0))
        {
            logfile.write("若是增量抽取，inc_file_name和conn_str1必二选一，不能都为空。\n");
            return false;
        }
    }

    return true;
}

/**
 * @brief 判断当前时间是否在程序运行的时间段内
 */
bool is_in_start_time()
{
    if(strlen(st_arg.start_time) != 0)
    {
        // 获取当前时间（小时）
        string str_hh24 = l_time_1("hh24");
        // 若当前时间不在指定的时间段内，则返回false
        if(strstr(st_arg.start_time, str_hh24.c_str()) == NULL)
            return false;
    }
    return true;
}

/**
 * @brief 数据抽取的主函数
 */
bool _dminingoracle()
{
    SqlStatement stmt(&conn);
    stmt.prepare(st_arg.select_sql);

    // 存储结果集的数组
    string field_value[field_name.size()];
    for(int i = 1; i <= field_name.size(); i++)
    {
        stmt.bindout(i, field_value[i - 1], stoi(field_len[i - 1]));
    }

    // 若增量抽取，绑定增量字段
    if(strlen(st_arg.inc_field) != 0)
        stmt.bindin(1, max_inc_value);

    if(stmt.execute() != 0)
    {
        logfile.write("stmt.execute(%s) failed: %s\n", stmt.sql(), stmt.message());
        return false;
    }

    pactive.upt_atime();

    // 输出的xml文件名
    string xml_file_name;
    // 输出xml文件的序号
    int seq =1;
    cofile out_xml_file;

    while (true)
    {
        // 从结果集获取数据，若获取失败，则跳出循环
        if(stmt.next() != 0)
            break;

        // 若文件不存在
        if(out_xml_file.is_open() == false)
        {
            s_format(xml_file_name, "%s/%s_%s_%s_%d.xml", \
                    st_arg.out_path, st_arg.begin_file_name, \
                    l_time_1("yyyymmddhh24miss").c_str(), \
                    st_arg.end_file_name, seq++);
            if(out_xml_file.open_file(xml_file_name) == false)
            {
                logfile.write("open %s failed.\n", xml_file_name.c_str());
                break;
            }
            out_xml_file.write_line("<data>\n");
        }

        // 按行写入数据
        for(int i = 1; i < field_name.size(); i++)
            out_xml_file.write_line("<%s>%s</%s>", field_name[i - 1].c_str(), field_value[i - 1].c_str(), field_name[i - 1].c_str());
        out_xml_file.write_line("<end>\n");

        // 若本次记录数超出最大记录数
        if(st_arg.max_count > 0 && stmt.rpc() % st_arg.max_count == 0)
        {
            out_xml_file.write_line("</data>\n");

            if(out_xml_file.close_and_rename() == false)
            {
                logfile.write("\"%s\" close_and_rename failed.\n", xml_file_name.c_str());
                return false;
            }
            logfile.write("generate xml file \"%s\", counts = %d\n", xml_file_name.c_str(), st_arg.max_count);

            pactive.upt_atime();
        }

        // 更新递增字段最大值
        if(strlen(st_arg.inc_field) != 0 && max_inc_value < stol(field_value[inc_field_pos]))
            max_inc_value = stol(field_value[inc_field_pos]);
    }

    // 结果集的最后部分
    if(out_xml_file.is_open() == true)
    {
        out_xml_file.write_line("</data>\n");
        if(out_xml_file.close_and_rename() == false)
        {
            logfile.write("\"%s\" close_and_rename failed.\n", xml_file_name.c_str());
            return false;
        }

        // 若单个xml文件不限制记录数上限
        if(st_arg.max_count == 0)
            logfile.write("generate xml file \"%s\", counts = %d\n", xml_file_name.c_str(), stmt.rpc());
        else
            logfile.write("generate xml file \"%s\", counts = %d\n", xml_file_name.c_str(), stmt.rpc() % st_arg.max_count);

    }
    
    if(stmt.rpc() > 0)
        write_inc_field();

    return true;
}

/**
 * @brief 从数据库表或文件中加载上次已抽取数据的最大值
 */
bool read_inc_field()
{
    max_inc_value = 0;

    // 若不是增量抽取
    if(strlen(st_arg.inc_field) == 0)
        return true;

    // 查找递增字段在结果集中的位置
    for(int i = 0; i < field_name.size(); i++)
    {
        if(field_name[i] == st_arg.inc_field)
        {
            inc_field_pos = i;
            break;
        }
    }

    if(inc_field_pos == -1)
    {
        logfile.write("递增字段: \"%s\" 在字段列表[%s]中不存在", st_arg.inc_field, st_arg.field_str);
        return false;
    }

    if(strlen(st_arg.conn_str1) != 0)
    {
        Connection conn1;
        if(conn1.connecttodb(st_arg.conn_str1, st_arg.charset) != 0)
        {
            logfile.write("connect database(%s) failed: ", st_arg.conn_str1, conn1.message());
        }
        SqlStatement stmt1(&conn1);
        stmt1.prepare("select maxincvalue from T_MAXINCVALUE where pname=:1");
        stmt1.bindin(1, st_arg.p_name);
        stmt1.bindout(1, max_inc_value);
        stmt1.execute();
        stmt1.next();
    }
    else
    {
        cifile ifile;
        if(ifile.open_file(st_arg.inc_file_name) == false)
        {
            // ?
            logfile.write("ifile.open_file(%s) failed.\n", st_arg.inc_file_name);
            return true;
        }

        string temp;
        ifile.read_line(temp);
        max_inc_value = stol(temp);
    }
    logfile.write("get last max_inc_value(%s = %ld)\n",st_arg.inc_field ,max_inc_value);

    return true;
}

/**
 * @brief 将已抽取数据最大值保存到数据库表或文件中
 */
bool write_inc_field()
{
    // 若非增量抽取
    if(strlen(st_arg.inc_field) == 0)
        return true;

    if(strlen(st_arg.conn_str1) != 0)
    {
        Connection conn1;
        if(conn1.connecttodb(st_arg.conn_str1, st_arg.charset) != 0)
        {
            logfile.write("connect database(%s) failed: %s\n",st_arg.conn_str1, conn1.message());
            return false;
        }
        logfile.write("connect database(%s) ok.\n",st_arg.conn_str1);
        
        SqlStatement stmt1(&conn1);
        stmt1.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
        stmt1.bindin(1, max_inc_value);
        stmt1.bindin(2, st_arg.p_name);
        if(stmt1.execute() != 0)
        {
            // 若表不存在
            if(stmt1.rc() == 942)
            {
                conn1.execute("CREATE TABLE T_MAXINCVALUE(pname varchar2(50), maxincvalue number(15), primary key(pname))");
                conn1.execute("INSERT INTO T_MAXINCVALUE VALUES('%s', '%ld')", st_arg.p_name, max_inc_value);
                conn1.commit();
                return true;
            }
            else
            {
                logfile.write("stmt1.execute(%s) failed: %s\n", stmt1.sql(), stmt1.message());
                return false;
            }
        }
        else
        {
            // 若记录不存在导致更新失败。，则直接插入
            if(stmt1.rpc() == 0)
            {
                conn1.execute("INSERT INTO T_MAXINCVALUE VALUES('%s', '%ld')", st_arg.p_name, max_inc_value);
            }
            conn1.commit();
        }
    }
    else
    {
        cofile ofile;
        if(ofile.open_file(st_arg.inc_file_name, false) == false)
        {
            logfile.write("ofile.open_file(%s) failed.\n", st_arg.inc_file_name);
            return false;
        }
        ofile.write_line("%ld", max_inc_value);
    }
    return true;
}
