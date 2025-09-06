/**
 * @brief  迁移表中的数据（数据清理）
*/

#include "_tools.h"
using namespace idc;

struct ST_Arg
{
    char conn_str[101];     // 数据库的连接参数。
    char table_name[31];        // 待迁移的表名。
    char to_table_name[31];     // 目标表名。
    char key_col[31];        // 待迁移的表的唯一键字段名。
    char where[1001];    // 待迁移的数据需要满足的条件。
    int  max_count;        // 执行一次SQL删除的记录数。
    char start_time[31];   // 程序运行的时间区间。
    int  time_out;             // 本程序运行时的超时时间。
    char p_name[51];      // 本程序运行时的程序名。
} st_arg;

clogfile logfile;
Connection conn;
cpactive pactive;

void _help();
void EXIT(int sig);
bool _xml_to_arg(const char *xml_buffer);
bool is_in_start_time();
bool _migratetable();

int main(int argc, char *argv[])
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

    cifile xml_file;
    if(xml_file.open_file(argv[2]) == false)
    {
        logfile.write("xml_file.open_file(%s) failed.\n", argv[2]);
        return -1;
    }
    char *xml_buffer = xml_file.read_all();
    if(_xml_to_arg(xml_buffer) == false)
    {
        logfile.write("_xml_to_arg() failed.\n");
        return -1;
    }

    if(is_in_start_time() == false)
    {
        logfile.write("current time(%s) is not in start time(%s).\n", l_time_1("hh24") , st_arg.start_time);
        return -1;
    }

    pactive.add_p_info(st_arg.time_out, st_arg.p_name);

    if(conn.connecttodb(st_arg.conn_str, "Simplified Chinese_China.AL32UTF8") != 0)
    {
        logfile.write("connect database(%s) failed: %s\n", st_arg.conn_str, conn.message());
        EXIT(1);
    }

    _migratetable();

    return true; 
}

void _help()
{
    print_dash_line(60);
    printf("Usage: /root/C++/selfC++Framework/tools/bin/migratetable log_file_name config_xml_file_name\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/migratetable /root/C++/selfC++Framework/log/migratetable.log /root/C++/selfC++Framework/tools/cpp/migratetable_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 3600 /root/C++/selfC++Framework/tools/bin/migratetable /root/C++/selfC++Framework/log/migratetable.log /root/C++/selfC++Framework/tools/cpp/migratetable_config.xml\n");
    printf("brief: 迁移表中的数据\n");
    printf("Param: \n");
    printf("    log_file_name 日志文件\n");
    printf("    config_xml_file_name 配置文件，参数说明如下\n");
    printf("        conn_str        数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("        table_name      待迁移数据表的表名。\n");
    printf("        to_table_name   目标数据表的表名。\n");
    printf("        key_col         待迁移数据表的唯一键字段名，可以用记录编号，如keyid，建议用rowid，效率最高。\n");
    printf("        where           待迁移的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("        max_count       执行一次SQL语句删除的记录数，建议在100-500之间。\n");
    printf("        start_time      程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间不运行。"\
                                        "如果start_time为空，本参数将失效，只要本程序启动就会执行数据迁移，"\
                                        "为了减少对数据库的压力，数据迁移一般在业务最闲的时候时进行。\n");
    printf("        time_out        本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("        p_name          进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n");
    print_dash_line(60);
}

void EXIT(int sig)
{
    logfile.write("Program exit, sig = %d\n", sig);
    exit(0);
}

bool _xml_to_arg(const char *xml_buffer)
{
    memset(&st_arg, 0, sizeof(st_arg));

    get_xml_buffer(xml_buffer, "conn_str", st_arg.conn_str, 100);
    if (strlen(st_arg.conn_str) == 0)
    {
        logfile.write("conn_str is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "table_name", st_arg.table_name, 30);
    if (strlen(st_arg.table_name) == 0)
    {
        logfile.write("table_name is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "to_table_name", st_arg.to_table_name, 30);
    if (strlen(st_arg.to_table_name) == 0)
    {
        logfile.write("to_table_name is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "key_col", st_arg.key_col, 30);
    if (strlen(st_arg.key_col) == 0)
    {
        logfile.write("key_col is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "where", st_arg.where, 1000);
    if (strlen(st_arg.where) == 0)
    {
        logfile.write("where is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "start_time", st_arg.start_time, 30);

    get_xml_buffer(xml_buffer, "max_count", st_arg.max_count);
    if (st_arg.max_count == 0)
    {
        logfile.write("max_count is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "time_out", st_arg.time_out);
    if (st_arg.time_out == 0)
    {
        logfile.write("time_out is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "p_name", st_arg.p_name, 50);
    if (strlen(st_arg.p_name) == 0)
    {
        logfile.write("p_name is null.\n");
        return false;
    }

    return true;
}

bool is_in_start_time()
{
    if(strlen(st_arg.start_time) != 0 && \
        strstr(st_arg.start_time, l_time_1("hh24").c_str()) == 0)
        return false;
     
    return true;
}

bool _migratetable()
{
    ctimer timer;
    char temp_value[21];    // 存放待删除的记录的唯一键的值

    // 准备提取表中数据的sql语句
    SqlStatement stmt_sel(&conn);
    stmt_sel.prepare("select %s from %s %s", st_arg.key_col, st_arg.table_name, st_arg.where);
    stmt_sel.bindout(1, temp_value, 20);

    // 准备删除表中数据的sql语句
    string del_sql = s_format("delete from %s where %s in (", st_arg.table_name, st_arg.key_col);
    for(int i = 0; i < st_arg.max_count; i++)
    {
        del_sql += s_format(":%lu,", i+1);
    }
    delete_rchr(del_sql, ',');
    del_sql += ")";

    // 存放唯一键的值的数组
    char key_values[st_arg.max_count][21];

    // 准备删除数据的sql语句
    SqlStatement stmt_del(&conn);
    stmt_del.prepare(del_sql);
    for(int i = 0; i < st_arg.max_count; i++)
    {
        stmt_del.bindin(i+1, key_values[i], 20);
    }

    // 准备插入目的表的sql语句
    C_TableColumns tableColumns;
    tableColumns.all_cols(conn, st_arg.table_name);
    string ins_sql = s_format("insert into %s select %s from %s where %s in (", st_arg.to_table_name, tableColumns.m_all_cols.c_str(), st_arg.table_name, st_arg.key_col);
    for(int i = 0; i < st_arg.max_count; i++)
    {
        ins_sql += s_format(":%lu", i+1);
    }
    delete_rchr(ins_sql, ',');
    ins_sql += ")";

    SqlStatement stmt_ins(&conn);
    stmt_ins.prepare(ins_sql);
    for(int i = 0; i < st_arg.max_count; i++)
        stmt_ins.bindin(i+1, key_values[i], 20);

    if(stmt_sel.execute() != 0)
    {
        logfile.write("stmt_sel.execute() failed: %s\n", stmt_sel.message());
        return false;
    }

    int effective_count = 0;
    memset(key_values, 0, sizeof(key_values));

    while (true)
    {
        memset(temp_value, 0, sizeof(temp_value));
        if(stmt_sel.next() != 0)
            break;
        strcpy(key_values[effective_count], temp_value);
        effective_count++;

        // 若数组中的记录数达到了st_arg.max_count，则执行一次删除数据
        if(effective_count >= st_arg.max_count)
        {
            if(stmt_ins.execute() != 0)
            {
                logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
                return false;
            }

            if(stmt_del.execute() != 0)
            {
                logfile.write("stmt_del.execute() failed: %s\n", stmt_del.message());
                return false;
            }
            
            conn.commit();

            effective_count = 0;
            memset(key_values, 0, sizeof(key_values));
            pactive.upt_atime();
        }
    }

    // 若数组中仍有数据，则再执行一次删除操作
    if(effective_count > 0)
    {
        if(stmt_ins.execute() != 0)
        {
            logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
            return false;
        }

        if(stmt_del.execute() != 0)
        {
            logfile.write("stmt_del.execute() failed: %s\n", stmt_del.message());
            return false;
        }
        conn.commit();
    }

    if(stmt_del.rpc() > 0)
        logfile.write("migrate %d rows from table(%s) to table(%s), spend %.02f seconds.\n", stmt_sel.rpc(), st_arg.table_name, st_arg.to_table_name, timer.elapsed());
    
    return true;
}
