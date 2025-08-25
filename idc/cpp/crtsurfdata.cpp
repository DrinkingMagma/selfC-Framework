#include "_public.h"
using namespace idc;

// 存放站点信息的结构体
struct st_code
{
    char prov_name[31];         // 省份名称
    char obt_id[11];            // 站点ID
    char obt_name[31];          // 站点名称
    double lat;                 // 纬度
    double lon;                 // 经度
    double height;              // 高度
};
list<st_code> lt_st_code;

// 气象站观测数据的结构体
struct st_surfdata
{
    char obtid[11];          // 站点代码。
    char ddatetime[15];  // 数据时间：格式yyyymmddhh24miss，精确到分钟，秒固定填00。
    int  t;                         // 气温：单位，0.1摄氏度。
    int  p;                        // 气压：0.1百帕。
    int  u;                        // 相对湿度，0-100之间的值。
    int  wd;                     // 风向，0-360之间的值。
    int  wf;                      // 风速：单位0.1m/s
    int  r;                        // 降雨量：0.1mm。
    int  vis;                     // 能见度：0.1米。
};
list<struct st_surfdata>  lt_st_out_data;           // 存放观测数据的容器。
void crt_surf_data();                                  // 根据stlist容器中的站点参数，生成站点观测数据，存放在datalist中。

char str_ddatetime[15];   // 数据时间：格式yyyymmddhh24miss，精确到分钟，秒固定填00。


clogfile logfile;
cpactive pactive;

void EXIT(int sig);
bool load_data(const string &infile);
bool crt_surf_file(const string &out_path, const string &file_fmt);

int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        cout << "Using: ./crtsurfdata infile outpath logfile out_file_fmt" << endl;
        cout << "Example: /root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/idc/bin/crtsurfdata /root/C++/selfC++Framework/idc/ini/stcode.ini /root/C++/selfC++Framework/idc/output/surfdata/ /root/C++/selfC++Framework/idc/log/crtsurfdata.log csv,json,xml" << endl;

        cout << "brief: 根据气象站点参数文件随机生成站点观测数据文件" << endl;
        cout << "use procctl start crtsurfdata." << endl;
        cout << "infile: 气象站点参数文件名" << endl;
        cout << "outpath: 存放站点数据的目录" << endl;
        cout << "logfile: 日志文件名" << endl;
        cout << "out_file_fmt: 数据文件的格式（csv, json, xml）" << endl;

        return -1;
    }

    close_io_and_signal(false);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    pactive.add_p_info(5, "crtsurfdata");

    if(logfile.open(argv[3]) == false)
    {
        cout << "logfile.open(" << argv[3] << ") failed." << endl;
        return -1;
    }
    // logfile.clear();

    logfile.write("crtsurfdata started.\n");

    // 从文件中加载站点数据
    if(load_data(argv[1]) == false)
        EXIT(-1);

    // 根据站点数据生成站点观测数据
    memset(str_ddatetime, 0, sizeof(str_ddatetime));
    l_time(str_ddatetime, "yyyymmddhh24miss");
    // snprintf：自动处理结束符
    snprintf(str_ddatetime + 12, 3, "00");  // 3 表示缓冲区大小（含 \0）
    crt_surf_data();

    // 将站点观测数据保存到文件中
    if(strstr(argv[4], "csv") != 0)
        crt_surf_file(argv[2], "csv");
    if(strstr(argv[4], "xml") != 0)
        crt_surf_file(argv[2], "xml");
    if(strstr(argv[4], "json") != 0)
        crt_surf_file(argv[2], "json");

    logfile.write("crtsurfdata exit.\n");

    return 0; 
}

void EXIT(int sig)
{
    logfile.write("catch sig=%d, crtsurfdata exit.\n", sig);

    exit(0);
}

bool load_data(const string &infile)
{
    cifile in_file;
    if(in_file.open_file(infile) == false)
    {
        logfile.write("in_file.open_file(%s) failed.\n", infile.c_str());
        return false;
    }

    string str_buffer;
    st_code code;

    // 读取文件第一行（标题行）
    in_file.read_line(str_buffer);

    while(in_file.read_line(str_buffer))
    {
        ccmdstr cmdstr(str_buffer, ",");
        memset(&code, 0, sizeof(code));

        cmdstr.getvalue(0, code.prov_name, 30);
        cmdstr.getvalue(1, code.obt_id, 10);
        cmdstr.getvalue(2, code.obt_name, 30);
        cmdstr.getvalue(3, code.lat);
        cmdstr.getvalue(4, code.lon);
        cmdstr.getvalue(5, code.height);

        lt_st_code.push_back(code);
    }

    logfile.write("load_data(%s) sussess.\n", infile.c_str());
    logfile.write("共有%d个站点\n", lt_st_code.size());

    // cout << "共有" << lt_st_code.size() << "个站点" << endl;
    // cout << "站点信息如下：" << endl;
    // cout << "站点名称" << "\t" << "站点编号" << "\t" << "站点名称" << "\t" << "纬度" << "\t" << "经度" << "\t" << "高度" << endl;
    // for (auto &code : lt_st_code)
    // {
    //     cout << code.obt_name << "\t" << code.obt_id << "\t" << code.obt_name << "\t" << code.lat << "\t" << code.lon << "\t" << code.height << endl;
    //     break;
    // }
    return true;
}

void crt_surf_data()
{
    srand(time(nullptr));

    st_surfdata stsurfdata;

    for(auto &aa : lt_st_code)
    {
        memset(&stsurfdata, 0, sizeof(stsurfdata));

        strcpy(stsurfdata.obtid, aa.obt_id);                        // 站点代码。
        strcpy(stsurfdata.ddatetime,str_ddatetime);        // 数据时间。
        stsurfdata.t=rand()%350;                                    // 气温：单位，0.1摄氏度。   0-350之间。 可犯可不犯的错误不要犯。
        stsurfdata.p=rand()%265+10000;                       // 气压：0.1百帕
        stsurfdata.u=rand()%101;                                    // 相对湿度， 0-100之间的值。
        stsurfdata.wd=rand()%360;                                 // 风向，0-360之间的值。
        stsurfdata.wf=rand()%150;                                  // 风速：单位0.1m/s。
        stsurfdata.r=rand()%16;                                      // 降雨>量：0.1mm。
        stsurfdata.vis=rand()%5001+100000;                 // 能见度：0.1米。

        lt_st_out_data.push_back(stsurfdata);                            // 把观测数据的结构体放入datalist容器。
    }
    logfile.write("crt_surf_data() sussess.\n");
    // for (auto &aa : lt_st_out_data)
    // {
    //     // logfile.write("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n", aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
    //     printf("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n", aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
    //     break;
    // }
}


bool crt_surf_file(const string &out_path, const string &file_fmt)
{
    string out_filename = out_path + "/" + "SURF_ZH_" + str_ddatetime + "_" + to_string(getpid()) + "." + file_fmt;

    cofile out_file;

    if(!out_file.open_file(out_filename))
    {
        logfile.write("can not open file %s\n", out_filename.c_str());
        return false;
    }

    if(file_fmt == "csv")
        out_file.write_line("obtid,ddatetime,t,p,u,wd,wf,r,vis\n");
    if(file_fmt == "xml")
        out_file.write_line("<data>\n");
    if(file_fmt == "json")
        out_file.write_line("{\"data\":[\n");

    for(auto &aa : lt_st_out_data)
    {
        if(file_fmt == "csv")
        {
            out_file.write_line("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",
                                    aa.obtid, aa.ddatetime, aa.t / 10.0, aa.p / 10.0, aa.u, aa.wd, aa.wf / 10.0 , aa.r / 10.0, aa.vis / 10.0); 
        }
        else if(file_fmt == "xml")
        {
            out_file.write_line("<obtid>%s</obtid><ddatetime>%s</ddatetime>" \
                                    "<t>%.1f</t><p>%.1f</p><u>%d</u><wd>%d</wd>" \
                                    "<wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n", \
                                    aa.obtid, aa.ddatetime, aa.t / 10.0, aa.p / 10.0,\
                                     aa.u, aa.wd, aa.wf / 10.0 , aa.r / 10.0, aa.vis / 10.0);
        }
        else if(file_fmt == "json")
        {
            out_file.write_line("{\"obtid\":\"%s\",\"datetime\":\"%s\",\"t\":%.1f,\"p\":%.1f,\"u\":%d,\"wd\":%d,\"wf\":%.1f,\"r\":%.1f,\"vis\":%.1f}", \
                                    aa.obtid, aa.ddatetime, aa.t / 10.0, aa.p / 10.0,\
                                     aa.u, aa.wd, aa.wf / 10.0 , aa.r / 10.0, aa.vis / 10.0);

            static size_t i = 0;
            if( i != lt_st_out_data.size() - 1)
            {
                out_file.write_line(",\n");
                i++;
            }
            else
            {
                out_file.write_line("\n");
            }
        }
    }

    if(file_fmt == "xml") 
        out_file.write_line("</data>\n");
    else if(file_fmt == "json")
        out_file.write_line("]}\n");

    out_file.close_and_rename();

    logfile.write("end write_data_to_file, %s, %s, %d\n", out_filename.c_str(), file_fmt.c_str(), lt_st_out_data.size());

    return true;
}