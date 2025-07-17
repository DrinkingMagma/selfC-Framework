/**
 * ftplib.h
 * 作用：ftp客户端的类的声明文件
*/

#ifndef __FTP_H
#define __FTP_H 

#include "_public.h"
#include "ftplib.h"

namespace idc
{
    class cftpclient
    {
        private:
            netbuf *m_ftp_conn;     // ftp连接句柄
        public:
            unsigned int m_size;    // 文件大小，单位：字节
            string m_mtime;         // 文件的最后修改时间，格式：yyyymmddhh24miss

            // 存放login方法登录失败的原因
            bool m_connect_failed;      // 网络连接失败
            bool m_login_failed;        // 账号或密码错误、没有登录权限
            bool m_option_failed;       // 设置传输模式失败

            cftpclient();
            ~cftpclient();

            cftpclient(const cftpclient &) = delete;
            cftpclient &operator=(const cftpclient &) = delete;

            /**
             * @brief 初始化数据成员
            */
            void init_dada();

            /**
             * @brief 登录ftp服务器
             * @param host ftp服务器的ip地址和端口，示例：192.168.1.1:21
             * @param username 所登录的ftp服务器的用户名
             * @param password 所登录的ftp服务器的密码
             * @param imode 传输模式，FTPLIB_PASSIVE表示被动模式，FTPLIB_PORT表示主动模式(缺省为被动模式)
            */
            bool login(const string &host, const string  &username, const string &password, const int imode = FTPLIB_PASSIVE);

            /**
             * @brief 登出ftp服务器
            */
            bool logout();

            /**
             * @brief 获取ftp服务器上文件的最后修改时间
             * @param filename ftp服务器上的文件名
             * @note 获取到的时间格式为：yyyymmddhh24miss
             * @note 获取到的时间存放在m_mtime上
            */
            bool mtime(const string &filename);

            /**
             * @brief 获取ftp服务器上文件的大小
             * @param filename ftp服务器上的文件名
             * @note 获取到文件大小存放在m_size上
            */
            bool size(const string &filename);

            /**
             * @brief 改变ftp服务器的当前工作目录
             * @param dir ftp服务端的目录名
            */
            bool chdir(const string &dir);

            /**
             * @brief 在ftp服务端上创建目录
             * @param dir ftp服务端上待创建的目录名
            */
            bool mkdir(const string &dir);

            /**
             * @brief 删除ftp服务端上的目录
             * @param dir ftp服务端上待删除的目录名
             * @note 返回为false有几种可能：1)权限不足；2)目录不存在：3)目录非空
            */
            bool rmdir(const string &dir);

            /**
             * @brief 列出ftp服务端目录下的子目录名和文件名
             * @param dir ftp服务端上的目录名
             * @param list_filename 存放ftp服务端目录下的子目录名和文件名
             * @note 若列出ftp服务器当前目录，dir可以为""/"*"/"."。 注意，对于不规范的fpt服务器来说可能有差别
            */
            bool nlist(const string &dir,const string &list_filename);

            /**
             * @brief 从ftp上获取文件
             * @param remote_file ftp服务端上的文件名
             * @param local_file 保存到本地的文件名
             * @param b_check_mtime 文件传输时是否检查文件最后修改时间
             * @note 传输过程中采用临时文件命名法，在获取文件成功后，将临时文件改名为local_filename
            */
            bool get(const string &remote_filename, const string &local_filename, const bool b_check_mtime = true);

            /**
             * @brief 向ftp服务器发送文件
             * @param local_filename 本地文件名
             * @param remote_filename 保存在ftp服务器的文件名
             * @param b_check_mtime 文件传输时是否检查文件最后修改时间
             * @note 传输过程中采用临时文件命名法，在获取文件成功后，将临时文件改名为remote_filename
            */
            bool put(const string &local_filename, const string &remote_filename, const bool b_check_mtime = true);

            /**
             * @brief 删除ftp服务器上的文件
             * @param filename 文件名
            */
            bool ftpdelete(const string &filename);

            /**
             * @brief 重命名ftp服务器上的文件
             * @param src_filename 源文件名
             * @param dest_filename 目标文件名
            */
            bool ftprename(const string &src_filename, const string &dst_filename);

            /**
             * @brief 向ftp服务器发送site命令
             * @param cmd 命令
            */
            bool site(const string &cmd);

            /**
             * @brief 获取ftp服务器的响应
             * @note 获取服务器返回信息的最后一条(return a pointer to the last response received)
            */
            char *response();
    };
} // namespace idc

#endif