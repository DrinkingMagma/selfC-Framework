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
    char right_where[1001];               // 同步数据的条件。
    char left_where[1001];               // 同步数据的条件。
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