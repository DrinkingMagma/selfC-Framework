#ifndef __PUBLIC_HH
#define __PUBLIC_HH 1

#include "_cmpublic.h"

using namespace std;

namespace idc
{
    /*-------------------------------------------------------------------*/
    /* 注释分隔符*/

    /**
     * @brief 打印分隔线
     * @param length 分割线长度
     * @param ch 分割线字符
     * @note 自动换行
    */
    void print_line(int length, char ch='-');

    /**
     * @brief 打印指定长度的虚线
     * @param length 虚线长度
     * @note 自动换行
    */
    void print_dash_line(int length);

    /**
     * @brief 获取指定长度的分隔线字符串
     * @param length 分割线长度
     * @param ch 分割线字符， 默认为 '-'
     * @param b_is_wrap_line 是否添加换行符
     * @note 默认不添加换行符
    */
    void get_separator_line(string &separator_line, int length, char ch='-', bool b_is_wrap_line=false);


    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
    /* C++风格字符串操作的若干函数*/

    /**
     * @brief 删除字符串左边的指定字符
     * @param str 待处理的字符串
     * @param cc 待删除的字符，缺省为空格
     */
    char *delete_lchr(char *str, const int cc = ' ');
    string &delete_lchr(string &str, const int cc = ' ');

    /**
     * @brief 删除字符串右边的指定字符
     * @param str 待处理的字符串
     * @param cc 待删除的字符，缺省为空格
     */
    char *delete_rchr(char *str, const int cc = ' ');
    string &delete_rchr(string &str, const int cc = ' ');

    /**
     * @brief 删除字符串左右两边的指定字符
     * @param str 待处理的字符串
     * @param cc 待删除的左右两边字符，缺省为空格
     */
    char *delete_lrchr(char *str, const int cc = ' ');
    string &delete_lrchr(string &str, const int cc = ' ');

    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
    /* 将字符串转换为大/小写*/

    /**
     * @brief 将字符串转换为大写
     * @param str 待处理的字符串
     */
    char *to_upper(char *str);
    string &to_upper(string &str);

    /**
     * @brief 将字符串转换为小写
     * @param str 待处理的字符串
     */
    char *to_lower(char *str);
    string &to_lower(string &str);

    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
    /* 字符串替换函数*/

    /**
     * @brief 字符串替换函数
     * @param str 待处理的字符串
     * @param old_str 待替换的字符串
     * @param new_str 替换的字符串
     * @param bloop 是否循环替换，true为循环替换
     * @note 替换可能导致字符串变长，对于char*类型变量，需要考虑空间问题
     * @note 若new_str为空，表示删除old_str
     */
    bool replace_str(char *str, const string &old_str, const string &new_str, const bool bloop = false);
    bool replace_str(string &str, const string &old_str, const string &new_strtr2, const bool bloop = false);

    /**
     * @brief 从字符串中提取数字、符号和小数点并存放到另一个字符串中
     * @param str 源字符串
     * @param dest 目标字符串
     * @param bsigned 是否提取符号
     * @param bpoint 是否提取小数点
     * @note 对于第二个函数重载，str和dest可以为同一个函数
     */
    char *pick_number(const string &src, char *dest, const bool bsigned = false, const bool bdot = false);
    string &pick_number(const string &src, string &dest, const bool bsigned = false, const bool bdot = false);
    string pick_number(const string &src, const bool bsigned = false, const bool bdot = false);

    /**
     * @brief 匹配字符串
     * @param str 目标字符串
     * @param rules 匹配规则，'*'表示任意字符， 多个表达式用','分隔
     * @note 匹配规则时，自动忽略字母大小写
     */
    bool match_str(const string &str, const string &rules);
    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
    /* ccmdstr类用于拆分存在分隔符的字符串*/
    class ccmdstr
    {
    private:
        vector<string> m_cmd_str; // 存放拆分后的字符串

        ccmdstr(const ccmdstr &) = delete;            // 禁用拷贝构造函数
        ccmdstr &operator=(const ccmdstr &) = delete; // 禁用赋值函数
    public:
        /**
         * @brief 构造函数
         * @param buffer 目标字符串
         * @param sep_str 分割符
         * @param b_del_space 是否去除字符串前后空格
         */
        ccmdstr() {};
        ccmdstr(const string &buffer, const string sep_str, const bool b_del_space = false);

        /**
         * @brief 重载[]运算符，获取指定索引的参数
         * @param ii 索引
         * @return 参数
         */
        const string &operator[](int ii) const
        {
            return m_cmd_str[ii];
        }

        /**
         * @brief 将字符串拆分到m_cmd_str中
         * @param buffer 目标字符串
         * @param sep_str 分割符
         * @param b_del_space 是否去除字符串前后空格
         */
        void split_to_cmd(const string &buffer, const string &sep_str, const bool b_del_space = false);

        /**
         * @brief 获取拆分后的字段个数
         */
        int size() const { return m_cmd_str.size(); };

        /**
         * @brief 获取字段内容
         * @param ii 字段索引
         * @param value 传入变量地址，用于存放字段内容
         * @param i_len 获取字段内容的长度
         * @return true 成功；false 失败（索引越界）
         * @note 需要确保i_len小于等于value的空间长度
         */
        bool getvalue(const int ii, string &value, const int i_len = 0) const; // C++风格字符串。
        bool getvalue(const int ii, char *value, const int i_len = 0) const;   // C风格字符串，ilen缺省值为0-全部长度。
        bool getvalue(const int ii, int &value) const;                         // int整数。
        bool getvalue(const int ii, unsigned int &value) const;                // unsigned int整数。
        bool getvalue(const int ii, long &value) const;                        // long整数。
        bool getvalue(const int ii, unsigned long &value) const;               // unsigned long整数。
        bool getvalue(const int ii, double &value) const;                      // 双精度double。
        bool getvalue(const int ii, float &value) const;                       // 单精度float。
        bool getvalue(const int ii, bool &value) const;

        /**
         * @brief 析构函数。
         */
        ~ccmdstr(); // bool型。
    };

    /**
     * @brief 重载<<运算符，输出ccmdstr::m_cmd_str中的内容
     * @param os 输出流
     * @param cc ccmdstr对象
     */
    ostream &operator<<(std::ostream &os, const ccmdstr &cc);

    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
    /* 解析xml格式字符串的函数族*/
    /**
     * @brief 解析xml格式字符串
     * @param xml_buffer 待解析的xml格式字符串
     * @param field_name 待解析的字段名
     * @param value 传入变量的地址，用于存放字段内容。支持bool、int、insigned int、
     *             long、unsigned long、double和char[]
     * @param i_len 字符串的长度，为0表示自动获取字符串长度
     * @return true 表示成功，false 表示失败（field_name字段不存在）
     * @note xml格式字符串的格式为：
     *  <file_name>/tmp/_public.h</file_name><mtime>2019-05-05 09:05:05</mtime><size>123</size>
     * @note 当value为char[]时，需要确保value的长度足够大，否则会出错。也可以用i_len限定获取的字段内容的长度
     */
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, string &value, int i_len = 0);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, char *value, int i_len = 0);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, bool &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, int &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, unsigned int &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, long &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, unsigned long &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, double &value);
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, float &value);

    /*-------------------------------------------------------------------*/
    /* C++格式化输出函数模版*/
    /**
     * @brief C++格式化输出函数模版
     * @param str 输出字符串
     * @param fmt 格式化字符串
     * @param args 参数
    */
    template <typename... Args>
    bool s_format(string &str, const char *fmt, Args... args)
    {
        // 确定需要分配的内存大小
        int len = snprintf(nullptr, 0, fmt, args...);
        if(len < 0)
            return false;
        if(len == 0)
            return true;

        str.resize(len);
        snprintf(&str[0], len + 1, fmt, args...);
        return true;
    }

    template <typename... Args>
    string s_format(const char *fmt, Args... args)
    {
        string str;
        int len = snprintf(nullptr, 0, fmt, args...);
        if(len < 0)
            return str;
        if(len == 0)
            return str;

        str.resize(len);
        snprintf(&str[0], len + 1, fmt, args...);
        return str;
    }

    /*-------------------------------------------------------------------*/
    /* 时间操作的若干函数*/

    /**
     * @brief 获取操作系统时间（用字符串表示）
     * @param str_time 获取到的时间
     * @param fmt 输出的时间格式
     * @param time_tvl 时间偏移量，单位为秒，可为负数
     * @note fmt格式含义：yyyy-年份；mm-月份；dd-日期；hh24-小时；mi-分钟；ss-秒
     * @note 支持格式： 1. yyyy-mm-dd hh24:mi:ss（缺省）
     *                  2. yyyymmddhh24miss
     *                  3. yyyy-mm-dd
     *                  4. yyyymmdd
     *                  5. hh24:mi:ss
     *                  6. hh24miss
     *                  7. hh24:mi
     *                  8. hh24mi
     *                  9. hh24
     *                  10. mi
     * @note 时间会自动在前面补0
     * @note 若fmt与上述格式都不匹配，str_time为空
    */
    string& l_time(string& str_time, const string& fmt = "", const int time_tvl = 0);
    char* l_time(char *str_time, const string& fmt = "", const int time_tvl = 0);
    string l_time_1(const string& fmt = "", const int time_tvl = 0);

    /**
     * @brief 将整数表示的时间转换为字符串表示
     * @param t_time 整数表示的时间
     * @param str_time 字符串表示的时间
     * @param fmt 时间格式
    */
    string& time_to_str(const time_t t_time, string &str_time, const string& fmt = "");
    char* time_to_str(const time_t t_time, char *str_time, const string& fmt = "");
    string time_to_str_1(const time_t t_time, const string& fmt = "");

    /**
     * @brief 将字符串表示的时间转换为整数表示
     * @param str_time 字符串表示的时间，格式不限，但需要包括yyyymmddhh24miss，顺序和数量不可变
     * @return 整数表示的时间，错误返回-1
    */
    time_t str_to_time(const string& str_time);

   /**
    * @brief 将字符串表示的时间加上偏移的秒数
    * @param in_time 输入的字符串时间格式
    * @param out_time 输出的，带偏移的字符串时间格式
    * @param time_tvl 偏移的秒数，可为负数
    * @param fmt 输出的时间格式
    * @note in_timr和out_time参数可以是同一个变量的地址，若调用失败，out_time会被清空
   */
   bool add_time(const string &in_time, char* out_time, const int time_tvl, const string &fmt = "");
   bool add_time(const string &in_time, string &out_time, const int time_tvl, const string &fmt = "");

    /*-------------------------------------------------------------------*/
    /* 精确到微秒的计时器类*/
    class ctimer
    {
    private:
        struct timeval m_start;     // 计时开始的时间点
        struct timeval m_end;       // 计时结束的时间点
    public:
        ctimer();
        
        /**
         * @brief 开始计时
        */
        void start();

        /**
         * @brief 计算逝去的时间，单位：秒。小数点后是微秒
         * @note 每调用一次该方法，重新开始计时
        */
        double elapsed();
    };

    /*-------------------------------------------------------------------*/
    /**
     * @brief 根据绝对路径的文件名或目录名逐级创建目录
     * @param path_or_filename 绝对路径的文件名或目录名
     * @param b_is_filename 是否是文件名
     * @return 创建成功返回true，否则返回false
     * @note 创建失败的原因：1)权限不足；2)路径中包含非法字符；3)磁盘空间不足
    */
    bool new_dir(const string &path_or_filename, bool b_is_filename = true);

    /*-------------------------------------------------------------------*/
    /* 文件操作相关函数*/

    /**
     * @brief 重命名文件，类似mv命令
     * @param src_filename 源文件名
     * @param dst_filename 目标文件名
     * @return true:成功，false:失败
     * @note 失败原因：1)权限不足；2)磁盘空间不足；3)源文件和目标文件不在一个磁盘上
     * @note 在重命名文件之前，会自动创建目标文件所在的目录
    */
    bool rename_file(const string &src_filename, const string &dst_filename);

    /**
     * @brief 复制文件，类似cp命令
     * @param src_filename 源文件名
     * @param dst_filename 目标文件名
     * @note 复制过程中采用临时文件名
     * @note 复制后的文件的时间与源文件相同
    */
    bool copy_file(const string &src_filename, const string &dst_filename);

    /**
     * @brief 获取文件大小
     * @param filename 文件名
     * @note 单位为字节
    */
    int get_file_size(const string &filename);

    /**
     * @brief 获取文件时间
     * @param filename 文件名
     * @param m_time 文件时间
     * @param fmt 输出的时间格式
    */
    bool file_mtime(const string &filename, char *mtime, const string &fmt = "yyyymmddhh24miss");
    bool file_mtime(const string &filename, string &mtime, const string &fmt = "yyyymmddhh24miss");


    /**
     * 重置文件的修改时间属性
     * @param filename 文件名
     * @param mtime 文件时间，格式不限，但要包括yyyymmddhh24miss且顺序不能变
    */
    bool set_mtime(const string &filename, const string &mtime);

    /*-------------------------------------------------------------------*/
    /* 获取某目录及其子目录的文件列表的类*/
    class cdir
    {
        private:
            vector<string> m_filelist;
            int m_pos;
            string m_fmt;

            cdir(const cdir &) = delete;                // 禁用拷贝构造函数
            cdir &operator=(const cdir &) = delete;     // 禁用赋值函数
        public:
            string m_dirname;       // 目录名
            string m_filename;       // 文件名，不包含目录名
            string m_ffilename;     // 绝对路径的文件名
            int m_filesize;         // 文件大小
            string m_mtime;         // 文件最后一次修改时间
            string m_ctime;         // 文件生成时间
            string m_atime;         // 文件最后一次访问时间

            cdir():m_pos(0), m_fmt("yyyymmddhh24miss"){};   // 构造函数

            /**
             * @brief 设置时间格式
             * @note 支持"yyyy-mm-dd hh24:mi:ss"和"yyyymmddhh24miss"格式
            */
            void set_fmt(const string &fmt);

            /**
             * @brief 获取目录列表，存放在m_filelist中
             * @param dir_name 目录名（绝对路径）
             * @param rules 文件名的匹配规则，不匹配的条件将被忽略
             * @param max_files 本次获取文件的最大数量
             * @param band_child 是否打开各级子目录
             * @param b_sort 是否按文件名排序
            */
            bool open_dir(const string &dir_name, const string &rules, const int max_files = 10000, const bool band_child = false, bool b_sort = false);
        private:
            /**
             * @brief 递归函数，被open_dir调用
            */
            bool _open_dir(const string &dir_name, const string &rules, const int max_files, const bool band_child);

        public:
            /**
             * @brief 从m_filelist中获取一条记录（文件名），同时获取该文件的大小、修改时间等信息
             * @note 调用open_dir()时，m_filelist容器被清空，m_pos归零，每调用一次read_dir()，m_pos加1
             * @note 当m_pos小于m_filelist.size()时，返回true
            */
            bool read_dir();

            /**
             * @brief 获取m_filelist中记录的条数
             * @return m_filelist中记录的条数
            */
            unsigned int get_size() { return m_filelist.size(); }

            ~cdir();
    };

    /*-------------------------------------------------------------------*/
    /* 写文件的类*/
    class cofile
    {
        private:
            ofstream f_out;         // 写入文件的对象
            string m_filename;      // 文件名
            string m_filename_tmp;  // 临时文件名，在m_filename后缀加上.tmp

        public:
            cofile() {};

            /**
             * @brief 检查文件是否打开
            */
            bool is_open() const { return f_out.is_open(); };

            /**
             * @brief 打开文件
             * @param filename 文件名
             * @param b_tmp 是否采用临时文件方案
             * @param mode 打开模式
             * @param ben_buffer 是否启用文件缓冲区
            */
            bool open_file(const string &filename, const bool b_tmp = true, const ios::openmode mode = ios::out, const bool ben_buffer = true);

            /**
             * @brief 将数据以文本的方式格式化输出到文件
             * @param fmt 格式化字符串
            */
            template<typename ...Args>
            bool write_line(const char* fmt, Args... args)
            {
                    if(f_out.is_open() == false)
                        return false;

                    f_out << s_format(fmt, args...);

                    return f_out.good();
            }

            /**
             * @brief 重载<<运算符，将数据以文本的方式输出到文件"
             * @note 换行只能使用'\n'，不能使用std::endl
            */
            template<typename T>
            cofile& operator<<(const T &data)
            {
                f_out << data;
                return *this;
            }

            /**
             * @brief 将二进制数据写入文件
             * @param buf 二进制数据
             * @param buf_size 二进制数据大小
            */
            bool write(void *buf, int buf_size);

            /**
             * @brief 关闭文件，并将临时文件名改为正式文件名
            */
            bool close_and_rename();

            /**
             * @brief 关闭文件，若有临时文件，则删除临时文件
            */
            void close();

            ~cofile() { close(); }
    };

    /*-------------------------------------------------------------------*/
    /* 读取文件的类*/
    class cifile
    {
        private:
            ifstream f_in;
            string m_filename;
        public:
            cifile() {};

            /**
             * @brief 判断文件是否打开
            */
           bool is_open() { return f_in.is_open(); }

           /**
            * @brief 打开文件
            * @param filename 文件名
            * @param mode 打开模式
           */
           bool open_file(const string &filename, const ios::openmode mode = ios::in);

            /**
             * @brief 以行的方式读取文件
             * @param buf 存储读取的行
             * @param end_sign 结束标志，缺省为空，没有结尾标志
            */
           bool read_line(string &buf, const string &end_sign = "");

            /**
             * @brief 读取二进制文件
             * @param buf 存储读取的二进制数据
             * @param buf_size 读取的二进制数据大小
             * @return 读取的字节数
            */
            int read(void *buf, int buf_size);

            /**
             * @brief 读取所有二进制数据
             * @return 读取的二进制数据
            */
            char* read_all();

            /**
             * @brief 关闭并删除文件
            */
           bool close_and_remove();

            /**
             * @brief 关闭文件
            */
           void close();

           ~cifile() { close(); }
    };

    /*-------------------------------------------------------------------*/
    /* 自旋锁*/
    /* 一种用于多线程同步的机制。自旋锁会让线程在获取锁失败时持续循环（自旋），
        而非进入休眠状态，从而避免线程上下文切换带来的开销*/
    class spinlock_mutex
    {
        private:
            // C++中唯一能保证无锁的原子类型
            atomic_flag flag;

            spinlock_mutex(const spinlock_mutex&) = delete;
            spinlock_mutex &operator=(const spinlock_mutex) = delete;
        public:
            /**
             * @brief 构造函数，设置锁在创建时处于未锁定状态
            */
            spinlock_mutex()
            {
                flag.clear();
            }
            
            /**
             * @brief 加锁。
             * @note 当锁处于为锁定状态时（test_and_set()返回false），线程获取锁并退出循环
             * @note 当锁处于锁定状态时（test_and_set()返回true），线程进入循环，并等待锁变为未锁定状态
            */
            void lock()
            {
                while (flag.test_and_set())
                    ;
            }

            /**
             * @brief 解锁
            */
            void unlock()
            {
                flag.clear();
            }
    };

    /*-------------------------------------------------------------------*/
    /* 日志文件类*/
    class clogfile
    {
        private:
            ofstream f_out;
            string m_filename;
            ios::openmode m_mode;
            bool m_backup;      // 是否自动切换日志
            int m_maxsize;      // 单个日志文件的最大大小，超出后自动切换日志文件
            bool m_ben_buffer;    // 是否启用缓冲区
            spinlock_mutex m_splock; // 自旋锁，用于在多线程的程序中各日志操作加锁
        public:
            /**
             * @brief 构造函数
             * @param maxsize 日志文件最大大小，默认100M
            */
            clogfile(int maxsize = 100 * 1024 * 1024) : m_maxsize(maxsize) {};
            

            /**
             * @brief 清空日志文件
            */
            bool clear();

            /**
             * @brief 打开日志文件
             * @param filename 日志文件名，若目录不存在会进行创建
             * @param mode 日志文件打开模式，默认为ios:app（追加）
             * @param b_backup 是否自动切换（备份），默认为true
             * @param b_buffer 是否启用缓冲区，默认为false
             * @note 在多进程的服务程序中，若多个进程共用一个日志文件，b_backup必须设置为false
             * @note 在多进程服务程序中，若多个进程往同一日志文件中写入大量日志，可能会出现混乱，多线程则不会
             * @note 只有同时写大量日志时才会出现混乱
             * @note 若需要避免，可以使用信号量加锁
            */
            bool open(const string &filename, const ios::openmode mode = ios::app, const bool b_backup = true, bool ben_buffer = false);

            /**
             * @brief 将日志内容以文本方式格式化输出到日志文件，并写入时间
            */
            template<typename... Args>
            bool write(const char* fmt, Args... args)
            {
                if(f_out.is_open() == false)
                    return false;

                backup();       // 判断是否需要切换日志文件

                m_splock.lock();
                f_out << l_time_1() << " " << s_format(fmt, args...);
                m_splock.unlock();

                return f_out.good();
            }

            /**
             * @brief 重载<<运算符，将日志内容以文本方式输出到日志文件中，不会添加时间
            */
            template<typename T>
            clogfile& operator<<(const T &value)
            {
                m_splock.lock();
                f_out << value;
                m_splock.unlock();

                return *this;
            }
        private:
            /**
             * @brief 日志文件备份
             * @note 若文件大小超出最大限制，则进行备份
             * @note 备份后的日志文件会在文件名后面加上备份的日期时间
             * @note 在多进程程序中，日志文件不可切换，多线程可以
            */
            bool backup();
        public:
            void close() {f_out.close(); };
            ~clogfile() {close();}
    };

    /*-------------------------------------------------------------------*/
    /* socket通讯相关类和函数*/

    /* tcp客户端*/
    class ctcpclient
    {
        private:
            int m_client_fd;
            string m_ip;
            int m_port;
        public:
            ctcpclient() : m_client_fd(-1), m_port(0) {}

            /** 
             * @brief 向服务器发起连接请求
             * @param ip 服务器ip
             * @param port 服务器端口
            */
            bool connect(const string &ip, const int port);

            /**
             * @brief 读取服务器发送的数据
             * @param buffer 存储数据的缓冲区
             * @param i_out_time 设置等待数据的超时时间
             * @note 超时有两种情况：1)等待超时；2)socket连接不可用
            */
            bool read(string &buffer, const int i_out_time = 0);
            bool read(void *buffer, const int i_buffer_size, const int i_out_time = 0);

            /**
             * @brief 向服务器发送数据
             * @param buffer 要发送的数据缓冲区
             * @param i_buffer_size 要发送的数据缓冲区大小
            */
            bool write(const string &buffer);
            bool write(const void *buffer, const int i_buffer_size);

            /**
             * @brief 关闭连接
            */
            void close_connect();

            ~ctcpclient();
    };

    /* tcp服务端*/
    class ctcpserver
    { 
        private:
            int m_listen_fd;
            int m_client_fd;
            struct sockaddr_in m_client_addr;
            struct sockaddr_in m_server_addr;
            int m_socket_addr_len;
        public:
            ctcpserver() : m_listen_fd(-1), m_client_fd(-1) {};

            /**
             * @brief 服务端初始化
             * @param port 端口号
             * @param back_log 最大连接数
            */
            bool init_server(const unsigned int port, const int back_log = 5);

            /**
             * @brief 从已连接队列中获取一个客户端连接，若队列为空则阻塞等待
            */
            bool accept_client();

            /**
             * @brief 获取客户端的ip地址
            */
            char *get_ip();

            /**
             * @brief 接受客户端发送的数据
             * @param buffer 存储数据的缓冲区
             * @param i_buffer_len 缓冲区大小
             * @param i_out_time 超时时间
            */
            bool read(string &buffer, const int i_out_time = 0);
            bool read(void *buffer, const int i_buffer_len, const int i_out_time = 0);

            /**
             * @brief 发送数据给客户端
             * @param buffer 要发送的数据
             * @param i_buffer_len 要发送的数据长度
            */
            bool write(const string &buffer);
            bool write(const void *buffer, const int i_buffer_len);

            /**
             * @brief 关闭监听的socket
             * @note 常用于多进程服务程序的子进程代码中
            */
            void close_listen();

            /**
             * @brief 关闭客户端的socket
             * @note 常用于多进程服务程序的父进程代码中
            */
            void close_client();

            ~ctcpserver();
    };

    /**
     * @brief 接受socket对端发送的数据
     * @param socket_fd 可用的socket连接
     * @param buffer 接收数据的缓冲区
     * @param i_buffer_size 缓冲区大小
     * @param timeout_ms 超时时间，单位秒
    */
    bool tcp_read(const int socket_fd, string &buffer, const int i_out_time = 0);
    bool tcp_read(const int socket_fd, void *buffer, const int i_buffer_size,const int i_out_time = 0);


    /**
     * @brief 向socket对端发送数据
     * @param socket_fd 可用的socket连接
     * @param buffer 发送数据的缓冲区
     * @param i_buffer_size 缓冲区大小
    */
    bool tcp_write(const int socket_fd, const string &buffer);
    bool tcp_write(const int socket_fd, const void *buffer, const int i_buffer_size);

    /**
     * @brief 从已经准备好的socket连接中读取数据
     * @param socket_fd 可用的socket连接
     * @param buffer 存放数据的地址
     * @param n 本次读取的长度
    */
    bool read_n(const int socket_fd, char *buffer, const size_t n);

    /**
     * @brief 向已经准备好的socket连接中写入数据
     * @param socket_fd 可用的socket连接
     * @param buffer 待写入数据的地址
     * @param n 本次写入的长度
    */
    bool write_n(const int socket_fd, const char *buffer, const size_t n);

    /*-------------------------------------------------------------------*/

    /**
     * @brief 忽略关闭全部的信号、关闭全部的IO、缺省只忽略信号，不忽略IO
    */
    void close_io_and_signal(bool b_close_io = false);

    /* 循环队列*/
    template <typename T, int Max_Length>
    class squeue
    {
        private:
            bool m_inited;
            T m_data[Max_Length];
            int m_head;
            int m_tail;
            int m_length;

            squeue(const squeue &) = delete;
            squeue &operator=(const squeue &) = delete;

        public:
            squeue()
            {
                init(); 
            }

            /**
             * @brief 初始化循环队列
             * @note 若用于共享内存的循环队列，不会调用构造函数，必须调用此函数进行初始化
            */
            void init()
            {
                if(m_inited != true)
                {
                    m_head = 0;
                    m_tail = Max_Length - 1;
                    m_length = 0;
                    memset(m_data, 0, sizeof(m_data));
                    m_inited = true;
                }
            }

            /**
             * @brief 添加数据
            */
            bool push(const T &data)
            {
                if(full() == true)
                {
                    cout << "squeue is full, push failed" << endl;
                    return false;
                }

                m_tail = (m_tail + 1) % Max_Length;
                m_data[m_tail] = data;
                m_length++;

                return true;
            }

            /**
             * @brief 获取循环队列长度
            */
            int size()
            {
                return m_length;
            }

            /**
             * @brief 判断循环队列是否为空
            */
           bool empty()
            {
                return m_length == 0;
            }

            /**
             * @brief 判断循环队列是否已满
            */
           bool full()
            {
                return m_length == Max_Length;
            }

            /**
             * @brief 查看队头元素的值，元素不出队
            */
            T& front()
            {
                return m_data[m_head];
            }

            /**
             * @brief 元素出队
             * @note 先进先出，出队的元素为队头元素
            */
            bool pop()
            {
                if(empty())
                    return false;
                
                m_head = (m_head + 1) % Max_Length;
                m_length--;

                return true;
            }

            /**
             * @brief 打印循环队列
             * @note 需要队列中的元素支持cout输出
            */
            void print_squeue()
            {
                for(int i = 0; i < size(); i++)
                {
                    cout << "m_data[" << (m_head + 1) % Max_Length << "] = " << m_data[(m_head + i) % Max_Length] << endl;
                }
            }
    };

    /**
     * Linux共享内存和信号量相关指令
     * 查看共享内存：ipcs -m
     * 删除共享内存：ipcrm -m <key>
     * 查看信号量：ipcs -s
     * 删除信号量：ipcrm -s <key>
    */

    /* 信号量*/
    class csemp
    {
        private:
            // 用于信号量操作的共同体
            union semun
            {
                int val;
                struct semid_ds *buf;
                unsigned short *arry;
            };

            int m_sem_id;       // 信号量id
            
            // 若全部修改过信号量的进程终止后，操作系统将把信号量恢复为初始值
            // 若设置为SEM_UNDO， 操作系统将跟踪进程对信号量的修改情况。一般用于互斥锁
            // 若设置为0，一般用于生产-消费者模型
            short m_sem_flg;

            csemp(const csemp &) = delete;
            csemp &operator=(const csemp &) = delete;
        public:
            csemp() : m_sem_id(-1) {}

            /**
             * @brief 若信号量已存在，获取信号量，否则创建并初始化
             * @note 若用于互斥锁，valur = 1, sem_flg = SEM_UNDO
             * @note 若用于生产-消费者模型，value = 0, sem_flg = 0
            */
            bool init(key_t key, unsigned short value = 1, short sem_flg = SEM_UNDO);

            /**
             * @brief P 操作，获取信号量
             * @param value每次获取的信号量的值
            */
            bool wait(short value = -1);

            /**
             * @brief V 操作，释放信号量
             * @param value每次释放的信号量的值
            */
            bool post(short value = 1);

            /**
             * @brief 获取信号量的ID
            */
            int get_m_sem_id();

            /**
             * @brief 获取信号量的值
            */
            int get_value();

            /**
             * @brief 销毁信号量
            */
            bool destroy();

            ~csemp();
    };

    /* 进程心跳信息的结构体*/
    struct st_proc_info
    {
        int pid = 0;            // 进程ID
        char p_name[51] = {0};  // 进程名称
        int out_time = 0;       // 超时时间，单位为秒
        time_t a_time = 0;      // 进程最后一次活动时间(最后一次心跳时间)

        st_proc_info() = default;   // 存在自定义的构造函数后，编译器将不再提供默认的构造函数，所以手动启用默认构造函数

        st_proc_info(const int in_pid, const string &in_p_name, const int in_out_time, const time_t in_a_time)
            : pid(in_pid), out_time(in_out_time), a_time(in_a_time)
        {
            strncpy(p_name, in_p_name.c_str(), sizeof(p_name) - 1);
        }
    };

    /* 宏定义：用于进程的心跳*/
    #define MAXNUMP 1000    // 进程最大数量
    #define SHMKEYP 0x5095  // 共享内存的key
    #define SEMKEYP 0x5095  // 信号量的key

    /* 进程心跳操作类*/
    class cpactive
    {
        private:
            int m_shm_id;           // 共享内存的id
            int m_pos;              // 当前进程在共享内存进程组中的位置
            st_proc_info *m_shm;    // 共享内存的指针， 指向共享内存的地址空间
        public:
            /**
             * @brief 构造函数，初始化成员变量
            */
            cpactive();

            /**
             * @brief 将当前进程信息写入共享内存
             * @param out_time 进程超时时间
             * @param p_name 进程名称
             * @param log_file 日志文件
            */
            bool add_p_info(const int out_time, const string &p_name = "", clogfile * log_file = nullptr);

            /**
             * @brief 更新共享内存进程组中当前进程的心跳时间
            */
            bool upt_atime();

            /**
             * @brief 析构函数，从贡献内存中删除当前进程的心跳记录
            */
            ~cpactive();
    };
} // namespace idc

#endif
