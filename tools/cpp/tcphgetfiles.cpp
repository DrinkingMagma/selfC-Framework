#include "_public.h"
using namespace idc;

// 程序运行的参数结构体
struct st_arg{
    int  client_type;               // 客户端类型：1-上传文件；2-下载文件，本程序固定填2
    char ip[31];                    // 服务器端的IP地址
    int  port;                      // 服务器端的端口号
    char server_path[256];          // 服务器端文件的存放路径
    int file_process_type;          // 文件下载成功后服务端文件的处理方式：1-不处理；2-删除；3-备份
    char server_path_bak[256];      // 服务器端文件的备份路径
    bool and_child;                 // 是否下载子目录下的文件
    char match_rules[256];          // 待下载文件名的匹配规则
    char client_path[256];          // 客户端文件的存放路径
    int time_val;                   // 扫描服务端目录文件的时间间隔。单位：秒
    int time_out;                   // 进程心跳的超时时间
    char process_name[51];          // 进程名，用于心跳检测
} arg;

clogfile logfile;