/*
 *  程序名：demo51.cpp，此程序演示采用开发框架的cftpclient类上传文件。
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

    // if(ftp.rmdir("/home/centos/tmp") == false)
    // {
    //     printf("ftp.rmdir(/home/centos/tmp) failed.\n");
    //     return -1;
    // }

    // 在ftp服务器上创建/home/centos/tmp，注意，如果目录已存在，会返回失败。
    if (ftp.mkdir("/home/centos/tmp")==false) { printf("ftp.mkdir() failed.\n"); return -1; }
  
    // 把ftp服务器上的工作目录切换到/home/centos/tmp
    if (ftp.chdir("/home/centos/tmp")==false) { printf("ftp.chdir() failed.\n"); return -1; }

    if (ftp.put("demo_50.cpp","demo_50.cpp")==true)
        printf("put demo_50.cpp ok.\n");  
    else
        printf("put demo_50.cpp failed.\n");  

    // 如果不调用chdir切换工作目录，以下代码采用全路径上传文件。
    // ftp.put("/project/public/demo/demo51.cpp","/home/centos/tmp/demo51.cpp");
}

