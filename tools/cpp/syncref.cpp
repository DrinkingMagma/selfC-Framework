/**
 * @brief 采用刷新的方法同步Oracle数据库之间的表
*/
#include "_tools.h"

struct ST_Arg
{
    char local_conn_str[101];         // 本地数据库的连接参数。
    char charset[51];                   // 数据库的字符集。
    char link_table_name[31];               // dblink指向的远程表名，如T_ZHOBTCODE1@db128。
    char local_table_name[31];            // 本地表名。
    char remote_cols[1001];        // 远程表的字段列表。
    char local_cols[1001];            // 本地表的字段列表。
    char remote_where[1001];               // 同步数据的条件。
    char local_where[1001];               // 同步数据的条件。
    int  sync_type;                     // 同步方式：1-不分批刷新；2-分批刷新。
    char remote_conn_str[101];   // 远程数据库的连接参数。
    char remote_table_name[31];       // 远程表名。
    char remote_key_col[31];       // 远程表的键值字段名。
    char local_key_col[31];           // 本地表的键值字段名。
    int  key_len;                        // 键值字段的长度。
    int  max_count;                   // 每批执行一次同步操作的记录数。
    int  time_out;                      // 本程序运行时的超时时间。
    char p_name[51];                 // 本程序运行时的程序名。
} st_arg;

clogfile logfile;
Connection conn_local;
Connection conn_remote;     // 分批次刷新还需要远程数据库连接
cpactive pactive;

void _help();
void EXIT(int sig);
bool _xml_to_arg(const char *xml_buffer);
bool _syncref();

int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        _help();
        return -1;
    }

    signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

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
    char *config_buffer = config_file.read_all();
    if(_xml_to_arg(config_buffer) == false)
    {
        logfile.write("_xml_to_arg() failed.\n");
        return -1;
    }

    // pactive.add_p_info(st_arg.time_out, st_arg.p_name);
    // 防止超时
    pactive.add_p_info(st_arg.time_out * 10000, st_arg.p_name);

    if(conn_local.connecttodb(st_arg.local_conn_str, st_arg.charset) != 0)
    {
        logfile.write("connect database(%s) failed: %s\n", st_arg.local_conn_str, conn_local.message());
        EXIT(-1);
    }

    // 若st_arg.remote_clos或st_arg.local_cols为空，则用st_arg.local_table_name表的全部列来填充
    if(strlen(st_arg.remote_cols) == 0 || strlen(st_arg.local_cols) == 0)
    {
        C_TableColumns tableColumns;

        if(tableColumns.all_cols(conn_local, st_arg.local_table_name) == false)
        {
            logfile.write("table(%s) is not exists.\n", st_arg.local_table_name);
            EXIT(-1);
        }

        if(strlen(st_arg.remote_cols) == 0)
            strcpy(st_arg.remote_cols, tableColumns.m_all_cols.c_str());
        if(strlen(st_arg.local_cols) == 0)
            strcpy(st_arg.local_cols, tableColumns.m_all_cols.c_str());
    }

    _syncref();

    return true; 
}

/**
 * @brief 程序帮助信息
*/
void _help()
{
    print_dash_line(60);
    printf("Usage: /root/C++/selfC++Framework/tools/bin/syncref log_file_name config_xml_file_name\n");
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/syncref /root/C++/selfC++Framework/log/syncref.log /root/C++/selfC++Framework/tools/cpp/syncref_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/syncref /root/C++/selfC++Framework/log/syncref.log /root/C++/selfC++Framework/tools/cpp/syncref_config_1.xml\n");
    printf("brief: 采用刷新的方式同步数据库之间的表\n");
    printf("Prama:\n");
    printf("    log_file_name 日志文件\n");
    printf("    config_file_name 配置文件，参数如下：\n");
    printf("        local_conn_str      本地数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("        charset             数据库的字符集，这个参数要与本地和远程数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("        link_table_name     dblink指向的远程表名，如T_ZHOBTCODE1@db128。\n");
    printf("        local_table_name    本地表名，如T_ZHOBTCODE2。\n");
    printf("        remote_cols         远程表的字段列表，用于填充在select和from之间，所以，remote_cols可以是真实的字段，"\
              "也可以是函数的返回值或者运算结果。如果本参数为空，就用local_table_name表的字段列表填充。\n");
    printf("        local_cols          本地表的字段列表，与remote_cols不同，它必须是真实存在的字段。如果本参数为空，"\
              "就用local_table_name表的字段列表填充。\n");
    printf("        remote_where         同步数据的条件，填充在远程表的查询语句之后，为空则表示同步全部的记录。\n");
    printf("        local_where          同步数据的条件，填充在本地表的删除语句之后，为空则表示同步全部的记录。\n");
    printf("        sync_type           同步方式：1-不分批刷新；2-分批刷新。\n");
    printf("        remote_conn_str     远程数据库的连接参数，格式与local_conn_str相同，当sync_type==2时有效。\n");
    printf("        remote_table_name   没有dblink的远程表名，当sync_type==2时有效。\n");
    printf("        remote_key_col      远程表的键值字段名，必须是唯一的，当sync_type==2时有效。\n");
    printf("        local_key_col       本地表的键值字段名，必须是唯一的，当sync_type==2时有效。\n");
    printf("        key_len             键值字段的长度，当sync_type==2时有效。\n");
    printf("        max_count           执行一次同步操作的记录数，当sync_type==2时有效。\n");
    printf("        time_out            本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
    printf("        p_name              本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
    printf("note：\n"\
              "1）remote_key_col和local_key_col字段的选取很重要，如果是自增字段，那么在远程表中数据生成后自增字段的值不可改变，否则同步会失败；\n"\
              "2）当远程表中存在delete操作时，无法分批刷新，因为远程表的记录被delete后就找不到了，无法从本地表中执行delete操作。\n");
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
 * @brief 将xml格式的字符串转换为参数结构体
 * @param xml_buffer xml字符串
*/
bool _xml_to_arg(const char *xml_buffer)
{
    memset(&st_arg, 0, sizeof(st_arg));

    // 本地数据库的连接参数，格式：ip,username,password,dbname,port。
    get_xml_buffer(xml_buffer, "local_conn_str", st_arg.local_conn_str, 100);
    if (strlen(st_arg.local_conn_str) == 0)
    {
        logfile.write("local_conn_str is null.\n");
        return false;
    }

    // 数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。
    get_xml_buffer(xml_buffer, "charset", st_arg.charset, 50);
    if (strlen(st_arg.charset) == 0)
    {
        logfile.write("charset is null.\n");
        return false;
    }

    // link_table_name表名。
    get_xml_buffer(xml_buffer, "link_table_name", st_arg.link_table_name, 30);
    if (strlen(st_arg.link_table_name) == 0)
    {
        logfile.write("link_table_name is null.\n");
        return false;
    }

    // 本地表名。
    get_xml_buffer(xml_buffer, "local_table_name", st_arg.local_table_name, 30);
    if (strlen(st_arg.local_table_name) == 0)
    {
        logfile.write("local_table_name is null.\n");
        return false;
    }

    // 远程表的字段列表，用于填充在select和from之间，所以，remote_cols可以是真实的字段，也可以是函数
    // 的返回值或者运算结果。如果本参数为空，将用local_table_name表的字段列表填充。
    get_xml_buffer(xml_buffer, "remote_cols", st_arg.remote_cols, 1000);

    // 本地表的字段列表，与remote_cols不同，它必须是真实存在的字段。如果本参数为空，将用local_table_name表的字段列表填充。
    get_xml_buffer(xml_buffer, "local_cols", st_arg.local_cols, 1000);

    // 同步数据的条件。
    get_xml_buffer(xml_buffer, "remote_where", st_arg.remote_where, 1000);
    get_xml_buffer(xml_buffer, "local_where", st_arg.local_where, 1000);

    // 同步方式：1-不分批刷新；2-分批刷新。
    get_xml_buffer(xml_buffer, "sync_type", st_arg.sync_type);
    if ((st_arg.sync_type != 1) && (st_arg.sync_type != 2))
    {
        logfile.write("sync_type is not in (1,2).\n");
        return false;
    }

    if (st_arg.sync_type == 2)
    {
        // 远程数据库的连接参数，格式与local_conn_str相同，当sync_type==2时有效。
        get_xml_buffer(xml_buffer, "remote_conn_str", st_arg.remote_conn_str, 100);
        if (strlen(st_arg.remote_conn_str) == 0)
        {
            logfile.write("remote_conn_str is null.\n");
            return false;
        }

        // 远程表名，当sync_type==2时有效。
        get_xml_buffer(xml_buffer, "remote_table_name", st_arg.remote_table_name, 30);
        if (strlen(st_arg.remote_table_name) == 0)
        {
            logfile.write("remote_table_name is null.\n");
            return false;
        }

        // 远程表的键值字段名，必须是唯一的，当sync_type==2时有效。
        get_xml_buffer(xml_buffer, "remote_key_col", st_arg.remote_key_col, 30);
        if (strlen(st_arg.remote_key_col) == 0)
        {
            logfile.write("remote_key_col is null.\n");
            return false;
        }

        // 本地表的键值字段名，必须是唯一的，当sync_type==2时有效。
        get_xml_buffer(xml_buffer, "local_key_col", st_arg.local_key_col, 30);
        if (strlen(st_arg.local_key_col) == 0)
        {
            logfile.write("local_key_col is null.\n");
            return false;
        }

        // 键值字段的大小。
        get_xml_buffer(xml_buffer, "key_len", st_arg.key_len);
        if (st_arg.key_len == 0)
        {
            logfile.write("key_len is null.\n");
            return false;
        }

        // 每批执行一次同步操作的记录数，当sync_type==2时有效。
        get_xml_buffer(xml_buffer, "max_count", st_arg.max_count);
        if (st_arg.max_count == 0)
        {
            logfile.write("max_count is null.\n");
            return false;
        }
    }

    // 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
    get_xml_buffer(xml_buffer, "time_out", st_arg.time_out);
    if (st_arg.time_out == 0)
    {
        logfile.write("time_out is null.\n");
        return false;
    }

    // 本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。
    get_xml_buffer(xml_buffer, "p_name", st_arg.p_name, 50);
    if (strlen(st_arg.p_name) == 0)
    {
        logfile.write("p_name is null.\n");
        return false;
    }

    return true;
}

/**
 * @brief 主函数
*/
bool _syncref()
{
    ctimer timer;
    
    SqlStatement stmt_del(&conn_local);
    SqlStatement stmt_ins(&conn_local);

    // 不分批刷新，适用于数据量不大（百万行以下）的场景
    // 远程表中的增加、删除和修改操作都可以同步到本地表
    if(st_arg.sync_type == 1)
    {
        logfile.write("sync data from table(%s) to table(%s) ......", st_arg.link_table_name, st_arg.local_table_name);

        // 先删除本地表中满足st_arg.local_where条件的记录
        stmt_del.prepare("delete from %s %s", st_arg.local_table_name, st_arg.local_where);
        if(stmt_del.execute() != 0)
        {
            logfile.write("stmt_del.execute() failed: %s\n", stmt_del.message());
            return false;
        }

        // 将远程表中满足st_arg.remote_where条件的记录插入到本地表
        stmt_ins.prepare("insert into %s(%s) select %s from %s %s", st_arg.local_table_name, st_arg.local_cols, st_arg.remote_cols, st_arg.link_table_name, st_arg.remote_where);
        if(stmt_ins.execute() != 0)
        {
            logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
            return false;
        }

        logfile << " " << stmt_ins.rpc() << " rows in " << timer.elapsed() << " seconds had been inserted\n";

        conn_local.commit();
    }

    // 分批刷新

    // 远程连接数据库
    if(conn_remote.connecttodb(st_arg.remote_conn_str, st_arg.charset) != 0)
    {
        logfile.write("connect database(%s) failed: %s\n", st_arg.remote_conn_str, conn_remote.message());
        return false;
    }

    // 从远程表中查找需要同步的键值
    SqlStatement stmt_sel(&conn_remote);
    stmt_sel.prepare("select %s from %s %s", st_arg.remote_key_col, st_arg.remote_table_name, st_arg.remote_where);
    char remote_key_value[st_arg.key_len + 1];  // 存放从远程表查到的需要绒布记录的键值
    stmt_sel.bindout(1, remote_key_value, 50);

    // 拼接绑定同步sql语句
    string bind_str;
    for(int i = 0; i < st_arg.max_count; i++)
    {
        bind_str += s_format(":%lu,", i + 1);
    }
    delete_rchr(bind_str, ',');

    char key_values[st_arg.max_count][st_arg.key_len + 1];

    // 准备删除本地表数据的sql语句，一次性删除st_arg.max_count条记录
    stmt_del.prepare("delete from %s where %s in (%s)", st_arg.local_table_name, st_arg.local_key_col, bind_str.c_str());
    for(int i = 0; i < st_arg.max_count; i++)
        stmt_del.bindin(i + 1, key_values[i]);

    // 准备插入本地表数据的sql语句，一次性插入st_arg.max_count条记录
    stmt_ins.prepare("insert into %s(%s) from %s where %s in (%s)", \
                st_arg.local_table_name, st_arg.local_cols, st_arg.remote_cols, \
                st_arg.link_table_name, st_arg.remote_key_col, bind_str.c_str());
    for(int i = 0; i < st_arg.max_count; i++)
        stmt_ins.bindin(i + 1, key_values[i]);

    int effective_count = 0;
    memset(key_values, 0, sizeof(key_values));

    logfile.write("sync data from table(%s) to table(%s) ...... ", st_arg.link_table_name, st_arg.local_table_name);

    if(stmt_sel.execute() != 0)
    {
        logfile << "failed.\n";
        logfile.write("stmt_sel.execute() failed: %s\n", stmt_sel.message());
        return false;
    }
    
    while (true)
    {
        if(stmt_sel.next() != 0)
            break;

        strcpy(key_values[effective_count], remote_key_value);
        effective_count++;

        // 每st_arg.maxcount条记录进行一次同步
        if(effective_count >= st_arg.max_count)
        {
            if(stmt_del.execute() != 0)
            {
                logfile << "failed.\n";
                logfile.write("stmt_del.execute() failed: %s\n", stmt_del.message());
                return false;
            }

            if(stmt_ins.execute() != 0)
            {
                logfile << "failed.\n";
                logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
                return false;
            }

            conn_local.commit();

            effective_count = 0;
            memset(key_values, 0, sizeof(key_values));
            pactive.upt_atime();
        }
    }
    
    if(effective_count > 0)
    {
        if(stmt_del.execute() != 0)
        {
            logfile << "failed.\n";
            logfile.write("stmt_del.execute() failed: %s\n", stmt_del.message());
            return false;
        }

        if(stmt_ins.execute() != 0)
        {
            logfile << "failed.\n";
            logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
            return false;
        }

        conn_local.commit();
    }
    logfile << "succeeded.\n";
    logfile.write("%s rows synced in %02f seconds.\n", stmt_sel.rpc(), timer.elapsed());

    return true;
}
