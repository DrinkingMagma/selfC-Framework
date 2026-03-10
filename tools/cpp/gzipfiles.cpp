/**
 * @file gzipfiles.cpp
 * @brief 文件压缩工具
 * @details 该程序用于压缩指定目录及其子目录中符合匹配规则且超过指定时间的文件
 */

#include "_public.h"
using namespace idc;

/// 进程活动对象
cpactive pactive;

/**
 * @brief 信号处理函数
 * @param sig 信号编号
 */
void EXIT(int sig);

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 0表示成功，-1表示失败
 */
int main(int argc, char *argv[])
{ 
    // 检查命令行参数
    if(argc != 4)
    {
        printf("\n"
                "Using: /root/C++/selfC++Framework/tools/bin/gzipfiles dir_path match_rules  out_time\n"
                "Example: /root/C++/selfC++Framework/tools/bin/gzipfiles /root/C++/selfC++Framework/idc/log \"*.log\" 0.01\n"
                "Example: /root/C++/selfC++Framework/tools/bin/gzipfiles /root/C++/selfC++Framework/idc/output/surfdata \"*.xml,*.csv,*.html\" 0.01\n"
                "\n\n"
                "dir_path: The directory address of the required compressed file.\n"
                "match_rules: The rules for matching the required compressed file.\n"
                "out_time: The most recent interval of files to be compressed. initialization value: 0.01 days.\n"
                "\n\n"
                "brief: This tool is used to compress files."
                "Note: The program will compress all files in the directory, including subdirectories.\n"
                "Note: The program will not wirite any log files."
                "Note: The program use \"/usr/bin/gzip\" to compress files.\n\n"
        );
        return -1;
    }

    // 关闭标准输入输出和信号
    close_io_and_signal(true);
    // 设置信号处理函数
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 添加进程活动信息
    pactive.add_p_info(120, "gzipfiles");

    // 计算过期时间
    string str_out_time = l_time_1("yyyymmddhh24miss", 0 - (int)atof(argv[3]) * 24 * 60 * 60);

    // 打开目录
    cdir dir;
    if(!dir.open_dir(argv[1], argv[2], 10000, true))
    {
        printf("Open dir [%s] failed.\n", argv[1]);
        return -1;
    }

    // 遍历目录中的文件
    while(dir.read_dir())
    {
        // 检查文件是否过期且不是已压缩文件
        if(dir.m_mtime < str_out_time && match_str(dir.m_filename, "*.gz") == false)
        {   
            // 构造 gzip 压缩命令，使用 -f 强制覆盖已存在的压缩文件
            // 1>/dev/null 2>/dev/null 将标准输出和错误输出重定向到空设备，避免输出到终端
            string str_cmd = "/usr/bin/gzip -f " + dir.m_ffilename + " 1>/dev/null 2>/dev/null";
            if(system(str_cmd.c_str()) == 0)
            {
                printf("Gzip [%s] ok.\n", dir.m_ffilename.c_str());
            }
            else
            {
                fprintf(stderr, "Gzip [%s] failed: %s\n", dir.m_ffilename.c_str(), strerror(errno));
            }
            // 更新进程活动时间
            pactive.upt_atime();
        }
    }

    return 0;
}

/**
 * @brief 信号处理函数
 * @param sig 信号编号
 */
void EXIT(int sig)
{
    printf("process exit, sig = %d.\n", sig);

    exit(0); 
}
