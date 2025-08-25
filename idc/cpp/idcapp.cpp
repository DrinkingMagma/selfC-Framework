
#include "idcapp.h"

bool C_ZHOBTMIND::split_buffer(const string &buffer, const bool b_is_xml)
{
    memset(&m_st_zhobtmind, 0, sizeof(m_st_zhobtmind));

    if(b_is_xml == true)
    {
        get_xml_buffer(buffer, "obtid", m_st_zhobtmind.obtid,5);
        get_xml_buffer(buffer, "ddatetime", m_st_zhobtmind.ddatetime, 14);
        char tmp[11];
        get_xml_buffer(buffer, "t", tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.t, 10, "%d", (int)(atof(tmp) * 10));
        get_xml_buffer(buffer, "p", tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.p, 10, "%d", (int)(atof(tmp) * 10));
        get_xml_buffer(buffer, "u", m_st_zhobtmind.u, 10);
        get_xml_buffer(buffer, "wd", m_st_zhobtmind.wd, 10);
        get_xml_buffer(buffer, "wf", tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.wf, 10, "%d", (int)(atof(tmp) * 10));
        get_xml_buffer(buffer, "r", tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.r, 10, "%d", (int)(atof(tmp) * 10));
        get_xml_buffer(buffer, "vis", tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.vis, 10, "%d", (int)(atof(tmp) * 10));
    }
    else
    {
        ccmdstr cmdstr;
        cmdstr.split_to_cmd(buffer, ",");
        cmdstr.getvalue(0, m_st_zhobtmind.obtid, 5);
        cmdstr.getvalue(1, m_st_zhobtmind.ddatetime, 14);
        char tmp[11];
        cmdstr.getvalue(2, tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.t, 10, "%d", (int)(atof(tmp) * 10));
        cmdstr.getvalue(3, tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.p, 10, "%d", (int)(atof(tmp) * 10));
        cmdstr.getvalue(4, m_st_zhobtmind.u, 10);
        cmdstr.getvalue(5, m_st_zhobtmind.wd, 10);
        cmdstr.getvalue(6, tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.wf, 10, "%d", (int)(atof(tmp) * 10));
        cmdstr.getvalue(7, tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.r, 10, "%d", (int)(atof(tmp) * 10));
        cmdstr.getvalue(8, tmp, 10);
        if(strlen(tmp) > 0)
            snprintf(m_st_zhobtmind.vis, 10, "%d", (int)(atof(tmp) * 10));
    }
    m_buffer = buffer;

    return true;
}

bool C_ZHOBTMIND::insert_to_t_zhobtmind()
{
    if(m_stmt.isopen() == false)
    {
        m_stmt.connect(&m_conn);
        m_stmt.prepare("insert into T_ZHOBTMIND(obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid) "\
                                   "values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND.nextval)");
        m_stmt.bindin(1,m_st_zhobtmind.obtid,5);
        m_stmt.bindin(2,m_st_zhobtmind.ddatetime,14);
        m_stmt.bindin(3,m_st_zhobtmind.t,10);
        m_stmt.bindin(4,m_st_zhobtmind.p,10);
        m_stmt.bindin(5,m_st_zhobtmind.u,10);
        m_stmt.bindin(6,m_st_zhobtmind.wd,10);
        m_stmt.bindin(7,m_st_zhobtmind.wf,10);
        m_stmt.bindin(8,m_st_zhobtmind.r,10);
        m_stmt.bindin(9,m_st_zhobtmind.vis,10);
    }

    if(m_stmt.execute() != 0)
    {
        // default is != 1
        if(m_stmt.rc() != 0)
        {
            m_logfile.write("buffer = %s\n", m_buffer.c_str());
            m_logfile.write("stmt.execute(insert) failed: \n%s\n%s\n", m_stmt.sql(), m_stmt.message());
        }
        return false;
    }
    return true;
}