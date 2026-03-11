/**
 * @file procctl.cpp
 * @brief 进程监控程序
 * @details 用于周期性执行指定程序或监控常驻内存程序
 */

#include "_public.h"
#include <sys/wait.h>
using namespace idc;

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 0表示成功，-1表示失败
 * @details 该程序用于周期性执行指定程序或监控常驻内存程序
 *          1. 当time_tvl大于等于5秒时，周期性执行指定程序
 *          2. 当time_tvl小于5秒时，监控常驻内存程序，确保其持续运行
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数，至少需要3个参数（程序名、时间间隔、被监控程序及其参数）
    if(argc < 3)
    {
        printf("Using: ./procctl time_tvl program argv ...\n");
        printf("Example: /root/C++/selfC++Framework/tools/bin/procctl 60 /root/C++/selfC++Framework/idc/bin/crtsurfdata /root/C++/selfC++Framework/idc/ini/stcode.ini /root/C++/selfC++Framework/idc/output/surfdata/ /root/C++/selfC++Framework/idc/log/crtsurfdata.log csv,json,xml\n");

        printf("the time_tvl unit is second.\n");
        printf("if the program is cyclical, time_tvl need to set cycle time.\n");
        printf("if the program is resident in memory, time_tvl need less than 5.\n");
        printf("note: if you need kill the program, please use \"kill -9 pid/programName\" command.\n");

        return -1;
    }

    // 关闭不需要的文件描述符，设置信号处理（不关闭标准输入/输出/错误）
    close_io_and_signal(false);

    // 创建第一个子进程，使当前进程成为后台守护进程
    if(fork() != 0)
        exit(0);

    // 设置SIGCHLD信号为默认处理方式，避免产生僵尸进程
    signal(SIGCHLD, SIG_DFL);

    // 构建被监控程序的参数列表（跳过前两个参数：程序名和时间间隔）
    char *p_argv[argc];
    for(int i = 2; i < argc; i++)
        p_argv[i - 2] = (char *)argv[i];
    p_argv[argc - 2] = nullptr;  // 参数列表以nullptr结尾

    // 主循环：持续监控并执行指定程序
    while (true)
    {
        // 创建第二个子进程用于执行目标程序
        if(fork() == 0)
        {
            // 在子进程中执行目标程序（替换当前进程映像）
            execv(argv[2], p_argv);
            
            // 如果execv执行失败（例如程序不存在），退出子进程
            exit(0);
        }
        else
        {
            // 父进程等待子进程结束（避免僵尸进程）
            wait(nullptr);
            
            // 等待指定的时间间隔后再次执行
            sleep(atoi(argv[1]));
        }
    }
    return 0; 
}