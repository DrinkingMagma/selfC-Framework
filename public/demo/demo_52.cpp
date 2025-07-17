/*
 *  程序名：demo52.cpp，此程序演示采用开发框架的cftpclient类下载文件。
 *  作者：吴从周
*/
#include "../_ftp.h"

using namespace idc;

int main(int argc,char *argv[])
{
    cftpclient ftp;

    // 登录远程ftp服务器，请改为你自己服务器的ip地址。
    if (ftp.login("192.168.59.132:21","centos","lzyou1011") == false)
    {
        printf("ftp.login(192.168.150.128:21,centos/lzyou1011) failed.\n"); return -1;
    }
    printf("ftp.login(192.168.150.128:21,centos/lzyou1011) successed.\n");

    // 把服务器上的/home/centos/tmp/demo_50.cpp下载到本地，存为/tmp/test/demo_50.cpp。
    // 如果本地的/tmp/test目录不存在，就创建它。
    if (ftp.get("/home/centos/tmp/demo_50.cpp","/tmp/test/demo_50.cpp")==false)
    { 
        printf("ftp.get() failed.\n"); return -1; 
    }

    printf("get /home/centos/tmp/demo_50.cpp ok.\n");  

    /*
    // 删除服务上的/home/centos/tmp/demo_50.cpp文件。
    if (ftp.ftpdelete("/home/centos/tmp/demo_50.cpp")==false) { printf("ftp.ftpdelete() failed.\n"); return -1; }

    printf("delete /home/centos/tmp/demo_50.cpp ok.\n");  

    // 删除服务器上的/home/centos/tmp目录，如果目录非空，删除将失败。
    if (ftp.rmdir("/home/centos/tmp")==false) { printf("ftp.rmdir() failed.\n"); return -1; }
    */
}

