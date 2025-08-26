#include "_public.h"
using namespace idc;

cpactive pactive;

void EXIT(int sig);

int main(int argc, char *argv[])
{ 
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

    close_io_and_signal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    pactive.add_p_info(120, "gzipfiles");

    string str_out_time = l_time_1("yyyymmddhh24miss", 0 - (int)atof(argv[3]) * 24 * 60 * 60);

    cdir dir;
    if(!dir.open_dir(argv[1], argv[2], 10000, true))
    {
        printf("Open dir [%s] failed.\n", argv[1]);
        return -1;
    }

    while(dir.read_dir())
    {
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
            pactive.upt_atime();
        }
    }

    return 0;
}

void EXIT(int sig)
{
    printf("process exit, sig = %d.\n", sig);

    exit(0); 
}
