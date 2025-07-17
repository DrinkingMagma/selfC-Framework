#include "_public.h"
using namespace idc;

// 程序运行的参数结构体
struct st_arg{
    int  client_type;               // 客户端类型：1-上传文件；2-下载文件，本程序固定填2
    char ip[31];                    // 服务器端的IP地址
    int  port;                      // 服务器端的端口号
    char server_path[256];          // 服务器端文件的存放路径
    int file_process_type;          // 文件下载成功后服务端文件的处理方式：1-不处理；2-删除；3-备份
    char server_path_bak[256];
    bool and_child;
    char match_rules[256];
    char client_path[256];
    int time_val;
    int time_out;
    char process_name[51];
} arg;

clogfile logfile;