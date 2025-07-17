#include "_public.h"
using namespace idc;

int main(int argc, char *argv[])
{
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

    close_io_and_signal(false);

    clogfile logfile;
    if(!logfile.open(argv[1]))
    {
        cout << "Can't open logfile:" << argv[1] << endl;
        return -1;
    }

    int shm_id = 0;
    if((shm_id = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(st_proc_info), 0666 | IPC_CREAT)) == -1)
    {
        logfile.write("create/get shared memory %x failed.\n", SHMKEYP);
        return false;
    }

    struct st_proc_info *shm = (struct st_proc_info *)shmat(shm_id, nullptr, 0);
    
    for(int i = 0; i < MAXNUMP; i++)
    {
        if(shm[i].pid == 0)
            continue;

        // 当向进程发送信号0时，若存在进程，kill返回0，否则返回-1
        int iret = kill(shm[i].pid, 0);
        if(iret == -1)
        {
            logfile.write("process pid = %d(name = %s) is not exist.\n", shm[i].pid, shm[i].p_name);
            memset(&shm[i], 0, sizeof(struct st_proc_info));
            continue;
        }

        time_t now = time(nullptr);

        if(now - shm[i].a_time < shm[i].out_time)
            continue;

        struct st_proc_info tmp_st_proc_info = shm[i];
        if(tmp_st_proc_info.pid == 0)
            continue;

        logfile.write("process pid = %d(name = %s) is timeout.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);

        kill(tmp_st_proc_info.pid, SIGTERM);

        for(int i = 0; i < 5; i++)
        {
            sleep(1);
            iret = kill(tmp_st_proc_info.pid, 0);
            if(iret == -1)
                break;
        }

        if(iret == -1)
        {
            logfile.write("process pid = %d(name = %s) is timeout, kill it normally.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);
        }
        else
        {
            kill(tmp_st_proc_info.pid, SIGKILL);
            logfile.write("process pid = %d(name = %s) is timeout, kill it force.\n", tmp_st_proc_info.pid, tmp_st_proc_info.p_name);
            memset(&shm[i], 0, sizeof(st_proc_info));
        }
    }
    shmdt(shm);

    return 0; 
}