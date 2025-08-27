#include "_tools.h"

C_TableColumns::C_TableColumns()
{
    init_data();
}


void C_TableColumns::init_data()
{
    m_v_all_cols.clear();
    m_v_pk_cols.clear();
    m_all_cols.clear();
    m_pk_cols.clear();
}

bool C_TableColumns::all_cols(Connection &conn, char *table_name)
{
    m_v_all_cols.clear();
    m_all_cols.clear();

    struct ST_Column st_column;

    SqlStatement stmt;
    stmt.connect(&conn);
    stmt.prepare("select lower(column_name), lower(data_type), data_length from USER_TAB_COLUMNS"\
                    "where table_name = upper(:1) order by column_id", table_name);
    stmt.bindin(1, table_name, 30);
    stmt.bindout(1, st_column.column_name, 30);
    stmt.bindout(2, st_column.data_type, 30);
    stmt.bindout(3, st_column.column_len);

    // 只有当数据库连接异常（网络断开、数据库出现问题）时，才会失败
    if(stmt.execute() != 0)
        return false;

    while (true)
    {
        memset(&st_column, 0, sizeof(st_column));

        if(stmt.next() != 0)
            break;

        // 各种字符串类型
        if(strcmp(st_column.data_type, "char") == 0 || \
            strcmp(st_column.data_type, "nchar") == 0 || \
            strcmp(st_column.data_type, "varchar2") == 0 || \
            strcmp(st_column.data_type, "nvarchar2") == 0)
        {
            strcpy(st_column.data_type, "char");
        }
        if(strcmp(st_column.data_type, "rowid") == 0)
        {
            strcpy(st_column.data_type, "char");
            st_column.column_len = 18;
        }

        // 时间类型
        if(strcmp(st_column.data_type, "date") == 0)
            st_column.column_len = 14;

        // 数字类型
        if(strcmp(st_column.data_type, "number") == 0 || \
            strcmp(st_column.data_type, "integer") == 0 || \
            strcmp(st_column.data_type, "float") == 0)
        {
            strcpy(st_column.data_type, "number");
        }

        // 若字段的数据类型不在以上范围，则忽略
        if(strcmp(st_column.data_type, "char") != 0 && \
            strcmp(st_column.data_type, "date") != 0 &&\
            strcmp(st_column.data_type, "number") != 0)
        {
            continue;
        }

        // 若字段类型为number，则字段长度设为22
        if(strcmp(st_column.data_type, "number") == 0)
        {
            st_column.column_len = 22;
        }

        // 拼接全部字段
        m_all_cols.append(st_column.column_name).append(",");

        m_v_all_cols.push_back(st_column);
    }

    if(stmt.rpc() > 0)
        delete_rchr(m_all_cols, ',');
    
    return true;
}

bool C_TableColumns::pk_cols(Connection &conn, char *table_name)
{
    m_v_pk_cols.clear();
    m_pk_cols.clear();

    struct ST_Column st_column;

    SqlStatement stmt;
    stmt.connect(&conn);
    // 从USER_CONS_COLUMNS和USER_CONSTRAINTS字典中获取表的主键字段
    // 注意：1）将结果集转换为小写；2）数据字典中的表名是大写
    stmt.prepare("select lower(column_name), position from USER_CONS_COLUMNS\
                where table_name = upper(:1)\
                    and constraint_name = (select constraint_name from USER_CONSTRAINTS\
                    where table_name = upper(:1) and constraint_type = 'p'\
                        and generated = 'USER NAME')\
                        order by position");
    stmt.bindin(1, table_name, 30);
    stmt.bindout(1, st_column.column_name, 30);
    stmt.bindout(2, st_column.pk_seq);

    if(stmt.execute() != 0)
        return false;

    while (true)
    {
        memset(&st_column, 0, sizeof(st_column));

        if(stmt.next() != 0)
            break;

        m_pk_cols.append(st_column.column_name).append(",");
        m_v_pk_cols.push_back(st_column);
    }
    if(stmt.rpc() > 0)
        delete_rchr(m_pk_cols, ',');

    // 更新m_v_all_cols中的pk_seq成员
    for(auto &ii : m_v_pk_cols)
    {
        for(auto &jj : m_v_all_cols)
        {
            if(strcmp(ii.column_name, jj.column_name) == 0)
            {
                jj.pk_seq = ii.pk_seq;
                break;
            }
        }
    }
    
    return true;
}
