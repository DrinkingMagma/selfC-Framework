/**
 * @file checkproc.cpp
 * @brief 检查后台服务程序是否超时的工具
 * @details 该程序用于检查后台服务程序是否超时，如果超时则终止它们
 */

#include "_public.h"
using namespace idc;

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 0表示成功，-1表示失败
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数
    if(argc != 2)
    {
        printf("Using:./checkproc logfile\n"
               "Example:/root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/checkproc /root/C++/selfC++Framework/log/checkproc.log\n"
               "logfile  the log file name for this program\n"
               "This program is check whether the background service program has timed out. If it has timed out, terminate it.\n"
               "Note: \n"
               "1. This program is started by procctl, and the recommended running cycle is 10 seconds.\n"
               "2. In order to avoid being killed by ordinary users, this program should be started by root user.\n"
               "3. You can only use \"killall -9 checkproc\" to terminate this program, if you want to stop this program.\n\n\n"
        );
        return -1;
    }

    // 关闭标准输入输出和信号
    close_io_and_signal(false);

    // 打开日志文件
    clogfile logfile;
    if(!logfile.open(argv[1]))
    {
        cout << "Can't open logfile:" << argv[1] << endl;
        return -1;
    }

    // 从共享内存中获取进程信息
    int shm_id = 0;
    if((shm_id = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(st_proc_info), 0666 | IPC_CREAT)) == -1)
    {
        logfile.write("create/get shared memory %x failed.\n", SHMKEYP);
        return -1;
    }

    // 附加共享内存
    struct st_proc_info *shm = (struct st_proc_info *)shmat(shm_id, nullptr, 0);
    
    // 遍历共享内存中的所有进程信息
    for(int i = 0; i < MAXNUMP; i++)
    {
        // 跳过空进程
        if(shm[i].pid == 0)
            continue;

        // 检查进程是否存在（发送信号0，若存在返回0，否则返回-1）
        int iret = kill(shm[i].pid, 0);
        if(iret == -1)
        {
            logfile.write("process pid = %d(name = %s) is not exist.\n", shm[i].pid, shm[i].p_name);
            memset(&shm[i], 0, sizeof(struct st_proc_info));
            continue;
        }

        // 获取当前时间
        time_t now = time(nullptr);

        // 检查进程是否超时
        if(now - shm[i].a_time < shm[i].out_time)
            continue;

        // 保存进程信息到临时变量
        struct st_proc_info tmp_st_proc_info = shm[i];
        if(tmp_st_proc_info.pid == 0)
            continue;

        // 记录超时信息
        logfile.write("process pid = %d(name = %s) is timeout.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);

        // 尝试正常终止进程
        kill(tmp_st_proc_info.pid, SIGTERM);

        // 等待5秒，检查进程是否终止
        for(int i = 0; i < 5; i++)
        {
            sleep(1);
            iret = kill(tmp_st_proc_info.pid, 0);
            if(iret == -1)
                break;
        }

        // 检查进程是否成功终止
        if(iret == -1)
        {
            logfile.write("process pid = %d(name = %s) is timeout, kill it normally.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);
        }
        else
        {
            // 强制终止进程
            kill(tmp_st_proc_info.pid, SIGKILL);
            logfile.write("process pid = %d(name = %s) is timeout, kill it force.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);
            // 清空共享内存中的进程信息
            memset(&shm[i], 0, sizeof(st_proc_info));
        }
    }
    
    // 分离共享内存
    shmdt(shm);

    return 0; 
}