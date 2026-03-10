/**
 * @file deletefiles.cpp
 * @brief 删除历史文件的工具
 * @details 该程序用于删除指定目录及其子目录中符合匹配规则且超过指定时间的文件
 */

#include "_public.h"
using namespace idc;

// 进程活动对象
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
                "Using: /root/C++/selfC++Framework/tools/bin/deletefiles file_dir match_rules out_time\n"
                "Example: /root/C++/selfC++Framework/tools/bin/deletefiles /root/C++/selfC++Framework/idc/output/surfdata/ \"*.xml,*.json,*.csv\" 0.01\n"
                "Example: /root/C++/selfC++Framework/tools/bin/deletefiles /root/C++/selfC++Framework/idc/ \"*.log\" 0.01\n"
                "\n"
                "file_dir: The directory of the files to be deleted.\n"
                "match_rules: The rules of the files to be deleted.\n"
                "out_time: The time of the files to be deleted, which is older than out_time. init 0.01 means 0.01 day.\n"
                "The default value is 0.01.\n"
                "\n"
                "This is a tool to delete history files.\n"
                "It will delete files which matched with rules in the file_dir and sub dir of file_dir, which are older than out_time.\n"
                "This program will not generate any log files.\n"
        );

        return -1;
    }

    // 关闭标准输入输出和信号
    close_io_and_signal(false);
    // 设置信号处理函数
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 添加进程活动信息
    pactive.add_p_info(30, "deletefiles");

    // 计算过期时间
    string str_out_time = l_time_1("yyyymmddhh24miss", 0 - (int)atof(argv[3]) * 24 * 60 * 60);

    // 打开目录
    cdir dir;
    if(dir.open_dir(argv[1], argv[2], 10000, true) == false)
    {
        printf("dir.open_dir(%s, %s) failed.\n", argv[1], argv[2]);
        return -1;
    }
    // 输出需要删除的文件数量
    printf("files number need be deleted is %d.\n", dir.get_size());

    // 遍历目录中的文件
    while (dir.read_dir() == true)
    {
        // 检查文件是否过期
        if(dir.m_mtime < str_out_time)
        {
            // 删除文件
            if(remove((dir.m_ffilename).c_str()) == 0)
            {
                printf("delete %s success.\n", dir.m_ffilename.c_str());
            }
            else
            {
                fprintf(stderr, "delete %s failed: %s\n", dir.m_ffilename.c_str(), strerror(errno));
            }
        }
    }
    // 输出删除完成信息
    printf("from \"%s\" delete \"%s\" finished.\n", argv[1], argv[2]);

    return 0; 
}

/**
 * @brief 信号处理函数
 * @param sig 信号编号
 */
void EXIT(int sig)
{
    printf("process exit, sig = %d\n", sig);

    exit(0); 
}