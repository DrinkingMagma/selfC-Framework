#include "_public.h"
using namespace idc;

cpactive pactive;

void EXIT(int sig);

int main(int argc, char *argv[])
{
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

    close_io_and_signal(false);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    pactive.add_p_info(30, "deletefiles");

    string str_out_time = l_time_1("yyyymmddhh24miss", 0 - (int)atof(argv[3]) * 24 * 60 * 60);

    cdir dir;
    if(dir.open_dir(argv[1], argv[2], 10000, true) == false)
    {
        printf("dir.open_dir(%s, %s) failed.\n", argv[1], argv[2]);
        return -1;
    }
    printf("files number need be deleted is %d.\n", dir.get_size());

    while (dir.read_dir() == true)
    {
        if(dir.m_mtime < str_out_time)
        {
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
    printf("from \"%s\" delete \"%s\" finished.\n", argv[1], argv[2]);

    return 0; 
}

void EXIT(int sig)
{
    printf("process exit, sig = %d\n", sig);

    exit(0); 
}