#include "_ftp.h"

namespace idc
{
    cftpclient::cftpclient()
    {
        m_ftp_conn = 0;

        init_data();

        FtpInit(); 

        m_connect_failed = false;
        m_login_failed = false;
        m_option_failed = false;
    }

    cftpclient::~cftpclient()
    {
        logout();
    }

    void cftpclient::init_data()
    {
        m_size = 0;
        m_mtime.clear();
    }

    bool cftpclient::login(const string &host, const string &username, const string &password, const int imode)
    {
        // 检查是否已有现存连接
        if(m_ftp_conn != 0)
        {
            // 发送QUIT命令优雅断开现有连接
            FtpQuit(m_ftp_conn);
            // 重置连接句柄
            m_ftp_conn = 0;
        }

        // 重置所有错误标志位
        m_login_failed = false;    // 登录失败标志
        m_connect_failed = false;  // 连接失败标志
        m_option_failed = false;   // 选项设置失败标志

        /* 第一步：建立TCP连接 */
        if(FtpConnect(host.c_str(), &m_ftp_conn) == false)
        {
            m_connect_failed = true;  // 标记连接失败
            return false;             // 返回失败
        }

        /* 第二步：用户认证 */
        if(FtpLogin(username.c_str(), password.c_str(), m_ftp_conn) == false)
        {
            m_login_failed = true;    // 标记登录失败
            return false;             // 返回失败
        }

        /* 第三步：设置连接模式 */
        if(FtpOptions(FTPLIB_CONNMODE, (long)imode, m_ftp_conn) == false)
        {
            m_option_failed = true;   // 标记选项设置失败
            return false;             // 返回失败
        }

        // 所有步骤成功完成
        return true;
    }

    bool cftpclient::logout()
    {
        if(m_ftp_conn == 0)
            return false;

        FtpQuit(m_ftp_conn);

        m_ftp_conn = 0;

        return true;
    }

    bool cftpclient::mtime(const string &filename)
    {
        if(m_ftp_conn == 0)
            return false;

        m_mtime.clear();

        string str_mtime;
        // UTC time length is 14(YYYYMMDDhhmmss)
        str_mtime.resize(14);

        if(FtpModDate(filename.c_str(), &str_mtime[0], 14, m_ftp_conn) == false)
            return false;

        // 北京时间比UTC时间快8个小时
        add_time(str_mtime, m_mtime, 0 + 8 * 60 * 60, "yyyymmddhh24miss");

        return true;
    }

    bool cftpclient::size(const string &filename)
    {
        if(m_ftp_conn == 0)
            return false;

        if(FtpSize(filename.c_str(), &m_size, FTPLIB_IMAGE, m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::chdir(const string &dir)
    {
        if(m_ftp_conn == 0)
            return false;

        if(FtpChdir(dir.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::mkdir(const string &dir)
    {
        if(m_ftp_conn == 0)
            return false;

        if(FtpMkdir(dir.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::rmdir(const string &dir)
    {
        if(m_ftp_conn == 0)
            return false;

        if(FtpRmdir(dir.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::nlist(const string &dir, const string &list_filename)
    {
        if(m_ftp_conn == 0)
            return false;

        new_dir(list_filename);

        if(FtpNlst(list_filename.c_str(), dir.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::get(const string &remote_filename, const string &local_filename, const bool b_check_mtime)
    { 
        if(m_ftp_conn == 0)
            return false;

        new_dir(local_filename);

        string str_local_filename = local_filename + ".tmp";

        if(mtime(remote_filename.c_str()) == false)
            return false;

        if(FtpGet(str_local_filename.c_str(), remote_filename.c_str(), FTPLIB_IMAGE, m_ftp_conn) == false)
            return false;

        // 确保在获取文件时，文件并未修改
        if(b_check_mtime == true)
        {
            string str_mtime = m_mtime;
            
            if(mtime(remote_filename) == false)
                return false;

            if(str_mtime != m_mtime)
                return false;
        }

        set_mtime(str_local_filename, m_mtime);

        if(rename(str_local_filename.c_str(), local_filename.c_str()) != 0)
            return false;

        m_size = get_file_size(local_filename);

        return true;
    }

    bool cftpclient::put(const string &local_filename, const string &remote_filename, const bool b_check_mtime)
    {
        if(m_ftp_conn == 0)
            return false;

        string str_remote_filename = remote_filename + ".tmp";

        string str_mtime_before;
        file_mtime(local_filename, str_mtime_before);

        if(FtpPut(local_filename.c_str(), str_remote_filename.c_str(), FTPLIB_IMAGE, m_ftp_conn) == false)
            return false;

        string str_mtime_after;
        file_mtime(local_filename, str_mtime_after);

        if(str_mtime_before != str_mtime_after)
            return false;

        if(FtpRename(str_remote_filename.c_str(), remote_filename.c_str(), m_ftp_conn) == false)
            return false;

        if(b_check_mtime == true)
        {
            if(size(remote_filename) == false)
                return false;

            if(m_size != get_file_size(local_filename))
            {
                ftpdelete(remote_filename);
                return false;
            }
        }
        return true;
    }

    bool cftpclient::ftpdelete(const string &filename)
    { 
        if(m_ftp_conn == 0)
            return false;

        if(FtpDelete(filename.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::ftprename(const string &src_filename, const string &dst_filename)
    {
        if(m_ftp_conn == 0)
            return false;

        if(FtpRename(src_filename.c_str(), dst_filename.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    bool cftpclient::site(const string &cmd)
    {
        if(m_ftp_conn == 0) 
            return false;
        
        if(FtpSite(cmd.c_str(), m_ftp_conn) == false)
            return false;

        return true;
    }

    char *cftpclient::response()
    {
        if(m_ftp_conn == 0)
            return 0;

        return FtpLastResponse(m_ftp_conn);
    }
}