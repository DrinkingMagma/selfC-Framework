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
    int upt_flag;               // 更新标志，1-更新，2-不更新
    char exec_sql[301];         // 处理xml文件前的sql处理语句
}st_xml_to_table;

clogfile logfile;
Connection conn;
cpactive pactive;
vector<struct ST_XmlToTable> v_st_xml_to_table; // 数据入库的参数的容器
C_TableColumns tableColumns;    // 获取表的全部及主键字段
string insert_sql;  // 插入数据的SQL语句
string update_sql;  // 更新数据的SQL语句
vector<string> v_columns_value;     // 存放从xml文件中解析出的字段值
SqlStatement stmt_ins, stmt_upt;
ctimer timer;
int total_count, ins_count, upt_count; //xml文件的总记录数，插入记录数和更新记录数

void _help();
bool _xml_to_arg(const char *xml_buffer);
void EXIT(int sig);
bool _xml_to_db();
int __xml_to_db(const string &full_file_name, const string &file_name);

bool load_xml_to_table();
bool find_xml_to_table(const string &xml_file_name);

void splice_ins_and_upt_sql();

void prepare_ins_and_upt_sql();
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
    printf("Example 1: /root/C++/selfC++Framework/tools/bin/xmltodb /root/C++/selfC++Framework/log/xmltodb.log /root/C++/selfC++Framework/tools/cpp/xmltodb_config.xml\n");
    printf("Example 2: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/xmltodb /root/C++/selfC++Framework/log/xmltodb.log /root/C++/selfC++Framework/tools/cpp/xmltodb_config.xml\n");
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
    memset(&st_arg, 0, sizeof(st_arg));

    get_xml_buffer(xml_buffer, "conn_str", st_arg.conn_str, 100);
    if (strlen(st_arg.conn_str) == 0)
    {
        logfile.write("conn_str is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "charset", st_arg.charset, 50);
    if (strlen(st_arg.charset) == 0)
    {
        logfile.write("charset is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "ini_file_name", st_arg.ini_file_name, 300);
    if (strlen(st_arg.ini_file_name) == 0)
    {
        logfile.write("ini_file_name is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "xml_path", st_arg.xml_path, 300);
    if (strlen(st_arg.xml_path) == 0)
    {
        logfile.write("xml_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "bak_xml_path", st_arg.bak_xml_path, 300);
    if (strlen(st_arg.bak_xml_path) == 0)
    {
        logfile.write("bak_xml_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "err_xml_path", st_arg.err_xml_path, 300);
    if (strlen(st_arg.err_xml_path) == 0)
    {
        logfile.write("err_xml_path is null.\n");
        return false;
    }

    get_xml_buffer(xml_buffer, "time_tvl", st_arg.time_tvl);
    if (st_arg.time_tvl < 2)
        st_arg.time_tvl = 2;
    if (st_arg.time_tvl > 30)
        st_arg.time_tvl = 30;

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

/**
 * @brief 程序退出的信号处理函数
*/
void EXIT(int sig)
{
    logfile.write("Program exit, sig = %d\n", sig);
    exit(0);
}

/**
 * @brief 数据入库主函数
*/
bool _xml_to_db()
{
    cdir dir;

    // 循环计数器，初始为50确保第一次进行循环时加载参数
    int count = 50;

    while (true)
    {
        if(count > 30)
        {
            if(load_xml_to_table() == false)
                return false;        
            count = 0;
        }
        else
            count++;

        // 打开starg.xml_path
        // 为保证先生成的xml文件先入库，打开目录时应按文件名排序
        if(dir.open_dir(st_arg.xml_path, "*.XML", 10000, false, true) == false)
        {
            logfile.write("_xml_to_db(): dir.open_dir(%s) failed.", st_arg.xml_path);
            return false;
        }

        if(conn.isopen() == false)
        {
            if(conn.connecttodb(st_arg.conn_str, st_arg.charset) != 0)
            {
                logfile.write("_xml_to_db(): conn.connecttodb(%s) failed: %s", st_arg.conn_str, conn.message());
                return false;
            }
            logfile.write("_xml_to_db(): connect database(%s) succeeded.", st_arg.conn_str);
        }

        while(true)
        {
            if(dir.read_dir() == false)
                break;

            logfile.write("processing file(%s) ...... ", dir.m_ffilename.c_str());

            int ret = __xml_to_db(dir.m_ffilename, dir.m_filename);

            pactive.upt_atime();

            if(ret == 0)
            {
                logfile << "succeeded.\t(" << st_xml_to_table.table_name << \
                    ", total count = " << total_count << ", insert count = " \
                    << ins_count << ", update count = " << upt_count << ", spent " \
                    << timer.elapsed() << " s)\n";

                if(xml_to_bak_or_err_dir(dir.m_ffilename, st_arg.xml_path, st_arg.bak_xml_path) == false)
                    return false;
            }

            if(ret == 1 || ret == 3 || ret == 4)
            {
                if(ret == 1)
                    logfile << "failed.\tparams error.\n";
                else if(ret == 3)
                    logfile << "failed.\ttable " << st_xml_to_table.table_name << " not exists.\n";
                else
                    logfile << "failed.\tsql statement execution failed before storage\n";
                if(xml_to_bak_or_err_dir(dir.m_ffilename, st_arg.xml_path, st_arg.err_xml_path))
                    return false;
            }
            else if(ret == 2)
            {
                logfile << "failed.\tdatabase error.\n";
                return false;
            }
            else if(ret == 5)
            {
                logfile << "failed.\topen file failed.";
                return false;
            }
        }
        if(dir.get_size() == 0)
            sleep(st_arg.time_tvl);

        pactive.upt_atime();
    }
    return true;
}


/**
 * @brief 处理xml文件的子函数
 * @param full_file_name 文件的完整路径
 * @param file_name 文件名
 * @note 成功返回0，失败返回其他数字
*/
int __xml_to_db(const string &full_file_name, const string &file_name)
{
    // 开始计时
    timer.start();
    total_count = ins_count = upt_count = 0;

    // 根据待入库的文件名查找入库配置参数，并得到对应的表名
    // 若无法找到xml文件对应的配置参数文件，返回1
    if(find_xml_to_table(file_name) == false)
    {
        return 1;
    }

    // 根据表名，读取数据字典，获取表的字段名和主键
    // 若获取失败，返回2
    if(tableColumns.all_cols(conn, st_xml_to_table.table_name) == false || \
        tableColumns.pk_cols(conn, st_xml_to_table.table_name) == false)
    {
        return 2;
    }

    // 根据表的字段名和主键，拼接插入和更新表的sql语句并绑定变量
    // 若表不存在或参数错误，返回3
    if(tableColumns.m_all_cols.size() == 0)
    {
        return 3;
    }

    // 拼接插入和更新表的sql语句
    splice_ins_and_upt_sql();

    // 准备sql语句并绑定输入变量
    prepare_ins_and_upt_sql();

    // 若处理xml之前，st_xml_to_table.exec_sql不为空，则先执行该sql语句
    // 执行失败则返回4
    if(exec_sql() == false)
    {
        return 4;
    }

    // 打开xml文件
    cifile xml_file;
    if(xml_file.open_file(full_file_name) == false)
    {
        conn.rollback();
        logfile.write("xml_file.open_file(%s) failed.\n", full_file_name);
        return 5;
    }
    string buffer;

    while (true)
    {
        if(xml_file.read_line(buffer, "<end/>") == false)
            break;
        total_count++;

        split_buffer(buffer);
        
        // 执行插入或更新的sql语句
        if(stmt_ins.execute() != 0)
        {
            // 若违反唯一性约束，表示记录已存在，则执行更新操作
            if(stmt_ins.rc() == 1)
            {
                if(st_xml_to_table.upt_flag == 1)
                {
                    if(stmt_upt.execute() != 0)
                    {
                        logfile.write("data update error: %s\n", buffer.c_str());
                        logfile.write("stmt_upt.execute() failed: %s\n", stmt_upt.message());
                    }
                    else
                        upt_count++;
                }
            }
            else
            {
                logfile.write("data insert error: %s\n", buffer.c_str());
                logfile.write("stmt_ins.execute() failed: %s\n", stmt_ins.message());
            }
        }
        else
            ins_count++;
    }
    conn.commit();
    return 0;
}
/**
 * @brief 将数据入库的配置文件（st_arg.ini_file_name）加载到v_st_xml_to_table中
*/
bool load_xml_to_table()
{
    v_st_xml_to_table.clear();

    cifile ini_i_file;
    if(ini_i_file.open_file(st_arg.ini_file_name) == false)
    {
        logfile.write("ini_i_file.open_file(%s) failed.\n", st_arg.ini_file_name);
        return false;
    }

    string buffer;

    while(true)
    {
        if(ini_i_file.read_line(buffer, "<endl/>") == false)
        {
            break;
        }

        memset(&st_xml_to_table, 0, sizeof(st_xml_to_table));

        get_xml_buffer(buffer, "file_name", st_xml_to_table.file_name, 100);
        get_xml_buffer(buffer, "table_name", st_xml_to_table.table_name, 30);
        get_xml_buffer(buffer, "upt_flag", st_xml_to_table.upt_flag);
        get_xml_buffer(buffer, "exec_sql", st_xml_to_table.exec_sql, 300);

        v_st_xml_to_table.push_back(st_xml_to_table);
    }
    logfile.write("load ini file(%s) succeeded.\n", st_arg.ini_file_name);

    return true;
}

/**
 * @brief 根据xml文件名，在v_st_xml_to_table中查找对应的数据入库配置，存放在st_xml_to_table中
 * @param xml_file_name 所需数据入库的xml文件名
*/
bool find_xml_to_table(const string &xml_file_name)
{
    for(auto &ii : v_st_xml_to_table)
    {
        if(match_str(xml_file_name, ii.file_name) == true)
        {
            st_xml_to_table = ii;
            return true;
        }
    }
    return false;
}

/**
 * @brief 拼接插入或更新sql语句，绑定输入变量
*/
void splice_ins_and_upt_sql()
{
    string insert_columns;
    string insert_values;
    int columns_seq = 1;

    for(auto &ii : tableColumns.m_v_all_cols)
    {
        // upttime字段缺省值为SYSDATE，不需要进行处理
        if(strcmp(ii.column_name, "upttime") == 0)
            continue;

        insert_columns.append(ii.column_name).append(",");
        
        if(strcmp(ii.column_name, "keyid") == 0)
        {
            insert_values.append(s_format("SEQ_%s.nextval", st_xml_to_table.table_name + 2)).append(",");
        }
        else
        {
            if(strcmp(ii.data_type, "date") == 0)
                insert_values.append(s_format("to_date(:%d, 'yyyymmddhh24miss')", columns_seq)).append(",");
            else
                insert_values.append(s_format(":%d", columns_seq)).append(",");
            columns_seq++;
        }
    }

    delete_rchr(insert_columns, ',');
    delete_rchr(insert_values, ',');

    // 拼接出完整的插入sql语句
    s_format(insert_sql, "insert into %s(%s) values(%s)", st_xml_to_table.table_name, insert_columns.c_str(), insert_values.c_str());
    // 若不需要更新，则不用拼接更新语句
    if(st_xml_to_table.upt_flag != 1)
        return;

    // 拼接更新sql语句
    update_sql = s_format("update %s set ", st_xml_to_table.table_name);
    columns_seq = 1;
    for(auto &ii : tableColumns.m_v_all_cols)
    {
        // 若为主键，则不需要update
        if(ii.pk_seq != 0)
            continue;

        // 若为keyid，也不需要更新
        if(strcmp(ii.column_name, "keyid") == 0)
            continue;

        // 若为upttime，直接赋值为SYSDATE
        if(strcmp(ii.column_name, "upttime") == 0)
        {
            update_sql.append("upttime=SYSDATE,");
            continue;
        }

        // 若为其他字段
        if(strcmp(ii.data_type, "date") != 0)
            update_sql.append(s_format("%s=:%d", ii.column_name, columns_seq));
        else
            update_sql.append(s_format("%s=TO_DATE(:%d, 'yyyymmddhh24miss')", ii.column_name, columns_seq));
        columns_seq++;
    }
    delete_rchr(update_sql, ',');

    // 拼接update后面部分
    // where 1 = 1：便于后续拼接
    update_sql.append(" where 1 = 1 ");
    for(auto &ii : tableColumns.m_v_all_cols)
    {
        // 若不是主键，则跳过
        if(ii.pk_seq == 0)
            continue;

        if(strcmp(ii.data_type, "date") != 0)
            update_sql.append(s_format(" and %s = :%d", ii.column_name, columns_seq));
        else
            update_sql.append(s_format(" and %s = to_date(:%d, 'yyyymmddhh24miss')", ii.column_name, columns_seq));
        columns_seq++;
    }
    return;
}

/**
 * @brief 准备插入和更新的sql语句，绑定输入变量
*/
void prepare_ins_and_upt_sql()
{
    // 为输入变量的数组分配内存
    v_columns_value.resize(tableColumns.m_all_cols.size());

    // 准备插入的sql语句并绑定输入变量
    stmt_ins.connect(&conn);
    stmt_ins.prepare(insert_sql);
    int columns_seq = 1;

    for(size_t i = 0; i < tableColumns.m_all_cols.size(); i++)
    {
        if(strcmp(tableColumns.m_v_all_cols[i].column_name, "upttime") == 0 || \
            strcmp(tableColumns.m_v_all_cols[i].column_name, "keyid") == 0)
            continue;
        stmt_ins.bindin(columns_seq++, v_columns_value[i], tableColumns.m_v_all_cols[i].column_len);
    }

    if(st_xml_to_table.upt_flag != 1)
        return;

    stmt_upt.connect(&conn);
    stmt_upt.prepare(update_sql);
    columns_seq = 1;
    for(size_t i = 0; i < tableColumns.m_v_all_cols.size(); i++)
    {
        if(tableColumns.m_v_all_cols[i].pk_seq != 0)
            continue;
        if(strcmp(tableColumns.m_v_all_cols[i].column_name, "upttime") == 0 || \
            strcmp(tableColumns.m_v_all_cols[i].column_name, "keyid") == 0)
            continue;
        stmt_upt.bindin(columns_seq++, v_columns_value[i], tableColumns.m_v_all_cols[i].column_len);
    }
    for(size_t i = 0; i < tableColumns.m_v_all_cols.size(); i++)
    {
        if(tableColumns.m_v_all_cols[i].pk_seq == 0)
            continue;
        stmt_upt.bindin(columns_seq++, v_columns_value[i], tableColumns.m_v_all_cols[i].column_len);
    }
}

/**
 * @brief 若处理xml文件前的sql处理语句（st_xml_to_table.exec_sql）不为空，则执行
*/
bool exec_sql()
{
    if(strlen(st_xml_to_table.exec_sql) == 0)
        return true;
    
    SqlStatement stmt;
    stmt.connect(&conn);
    stmt.prepare(st_xml_to_table.exec_sql);
    if(stmt.execute() != 0)
    {
        logfile.write("exec_sql() error: stmt.execute() failed: %s\n", stmt.message());
        return false;
    }
    return true;
}

/**
 * @brief 解析xml内容，存在在已绑定的输入变量v_column_value数组中
 * @param buffer xml内容
*/
void split_buffer(const string &buffer)
{
    string temp_buffer;

    for(size_t i = 0; i < tableColumns.m_v_all_cols.size(); i++)
    {
        // 临时变量用于防止调用移动构造函数和移动赋值函数时改变v_column_value数组中string的内部地址
        get_xml_buffer(buffer, tableColumns.m_v_all_cols[i].column_name, temp_buffer, tableColumns.m_v_all_cols[i].column_len);

        if(strcmp(tableColumns.m_v_all_cols[i].data_type, "date") == 0)
        {
            pick_number(temp_buffer, temp_buffer, false, false);
        }
        else if (strcmp(tableColumns.m_v_all_cols[i].data_type, "number") == 0)
        {
            pick_number(temp_buffer, temp_buffer, true, true);
        }
        else
        {
            v_columns_value[i] = temp_buffer.c_str();
        }
    }
    return;
}

/**
 * @brief 将xml文件移动到备份目录或错误目录中
 * @param full_file_name xml文件文件名
 * @param src_path 源文件地址
 * @param dst_path 目标文件地址
*/
bool xml_to_bak_or_err_dir(const string &full_file_name, const string &srv_path, const string &dst_path)
{
    string dst_file_name = full_file_name;

    replace_str(dst_file_name, srv_path, dst_path, false);

    if(rename_file(full_file_name, dst_file_name) == false)
    {
        logfile.write("xml_to_bak_or_err_dir() error: rename_file(%s, %s) failed.", full_file_name.c_str(), dst_file_name.c_str());
    }

    return true;
}
