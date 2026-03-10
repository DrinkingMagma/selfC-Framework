/**
 * @brief 共享平台项目公用函数和类的声明文件
*/

#ifndef IDCAPP_H
#define IDCAPP_H

#include "_public.h"
#include "_ooci.h"
using namespace idc;

/**
 * @brief 全国气象观测数据操作类
 * @details 该类用于操作全国气象观测数据，包括拆分文件数据、插入数据库等操作。
 */
class C_ZHOBTMIND
{
private:
    struct ST_ZHOBTMIND
    { 
        char obtid[6];            // 站点代码。
        char ddatetime[21];  // 数据时间，精确到分钟。
        char t[11];                 // 温度，单位：0.1摄氏度。
        char p[11];                // 气压，单位：0.1百帕。
        char u[11];                // 相对湿度，0-100之间的值。
        char wd[11];             // 风向，0-360之间的值。
        char wf[11];              // 风速：单位0.1m/s。
        char r[11];                // 降雨量：0.1mm。
        char vis[11];             // 能见度：0.1米。
    };

    Connection &m_conn;
    clogfile &m_logfile;

    SqlStatement m_stmt;
    string m_buffer;
    struct ST_ZHOBTMIND m_st_zhobtmind;
public:
    C_ZHOBTMIND(Connection &conn, clogfile &logfile) : m_conn(conn), m_logfile(logfile){}

    ~C_ZHOBTMIND(){}

    /**
     * @brief 将文件中的行数据拆分到m_st_zhobtmind结构体中
     * @param buffer 文件中的行数据
     * @param b_is_xml 是否是xml格式
    */
    bool split_buffer(const string &buffer, const bool b_is_xml = false);

    /**
     * @brief 将m_st_zhobtmind结构体中的数据插入T_ZHOBTMIND表中
    */
   bool insert_to_t_zhobtmind();
};


#endif