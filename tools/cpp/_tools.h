#ifndef _TOOLS_H
#define _TOOLS_H 

#include "_public.h"
#include "_ooci.h"
using namespace idc;

// 获取表中全部的列和主键列的信息的类
class C_TableColumns
{
    // 表的列信息结构体
    struct ST_Column
    {
        char column_name[31];   // 列名
        char data_type[31];     // 数据类型，分为number, date和char三大类
        int column_len;         // 长度，number固定取20，date固定取19，char长度由表结构决定
        int pk_seq;             // 主键序号，0表示非主键列
    };
public:
    vector<struct ST_Column> m_v_all_cols;  // 存放所有列信息
    vector<struct ST_Column> m_v_pk_cols;   // 存放主键列信息
    string m_all_cols;  // 全部字段名的列表，中间用半角逗号分隔
    string m_pk_cols;   // 主键字段名的列表，中间用半角逗号分隔

    C_TableColumns();

    /**
     * @brief 初始化成员变量
    */
    void init_data();

    /**
     * @brief 获取指定表的全部字段信息
     * @param conn 数据库连接对象
     * @param table_name 表名
    */
    bool all_cols(Connection &conn, char *table_name);

    /**
     * @brief 获取指定表主键字段信息
     * @param conn 数据库连接对象
     * @param table_name 表名
    */
   bool pk_cols(Connection &conn, char *table_name);
};

#endif