#include "_public.h"
using namespace idc;

cpactive pactive;      // 进程心跳，用全局对象（保证析构函数会被调用）。

void EXIT(int sig)     // 程序退出和信号2、15的处理函数。
{
    printf("sig=%d\n",sig);

    exit(0);
}

int main(int argc,char *argv[])
{
    if(argc!=3)
    {
        printf("Using: ./demo2 out_time cur_proc_running_time \n"
                "Example: ./demo2 20 10(process runs normally)\n"
                "Example: ./demo2 10 20(process timed out)\n"
                "Pram:\n"
                "out_time: process out time, unit: second.\n"
                "cur_proc_running_time: process running time, unit: second.\n"
        );
        return -1;
    }

    // 处理程序的退出信号。
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);
    close_io_and_signal(false);
    
    pactive.add_p_info(atoi(argv[1]),"demo02");  // 把当前进程的信息加入共享内存进程组中。

    while (1)
    {
        sleep(atoi(argv[2]));
        pactive.upt_atime(); // 更新进程的心跳。
    }

    return 0;
}
