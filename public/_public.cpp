#include "_public.h"

namespace idc
{
    void print_line(int length, char ch)
    {
        if(length <= 0)
        {
            printf("\n");
            return;
        }

        for(int i = 0; i < length; i++)
            putchar(ch);
        putchar('\n');
    }

    void print_dash_line(int length)
    {
        print_line(length, '-');
    }

    void get_separator_line(string &separator_line, int length, char ch, bool b_is_wrap_line)
    {
        separator_line.clear();

        if(length <= 0)
            return;

        separator_line = string(length, ch);

        if(b_is_wrap_line)
            separator_line += "\r\n";
    }

    char *delete_lchr(char *str, const int cc)
    {
        if (str == nullptr)
            return nullptr;

        char *p = str;
        while (*p == cc)
            p++;

        memmove(str, p, strlen(str) - (p - str) + 1); // memmove()将字符串中的部分内容向前移动，，'+1'用于包含字符串结束符'\0'

        return str;
    }

    string &delete_lchr(string &str, const int cc)
    {
        if (str.empty())
            return str;

        // 'find_if()'：在字符串中查找第一个不满足条件的字符串
        str.erase(str.begin(), find_if(str.begin(), str.end(), [cc](char ch)
                                       { return ch != cc; }));

        return str;
    }

    char *delete_rchr(char *str, const int cc)
    {
        if (str == nullptr)
            return nullptr;

        char *p = str;
        char *pis_cc = 0;

        // 遍历字符串，记录右边全是字符cc时的开始位置
        while (*p != 0)
        {
            if (*p == cc && pis_cc == 0)
                pis_cc = p;
            if (*p != cc)
                pis_cc = 0;
            p++;
        }

        if (pis_cc != 0)
            *pis_cc = 0; // 将该位置的值置为0（及'\0'的ASCII码），表示字符串结束

        return str;
    }

    string &delete_rchr(string &str, const int cc)
    {
        // 在字符串右边找到最后一个非cc字符
        auto pos = str.find_last_not_of(cc);

        if (pos != 0)
            str.erase(pos + 1);

        return str;
    }

    char *delete_lrchr(char *str, const int cc)
    {
        delete_lchr(str, cc);
        delete_rchr(str, cc);

        return str;
    }

    string &delete_lrchr(string &str, const int cc)
    {
        delete_lchr(str, cc);
        delete_rchr(str, cc);

        return str;
    }

    char *to_upper(char *str)
    {
        if (str == nullptr)
            return nullptr;

        char *p = str;
        while (*p != 0)
        {
            if (*p >= 'a' && *p <= 'z')
                *p -= 32;
            p++;
        }
        return str;
    }

    string &to_upper(string &str)
    {
        for (auto &cc : str)
        {
            if (cc >= 'a' && cc <= 'z')
                cc -= 32;
        }
        return str;
    }

    char *to_lower(char *str)
    {
        if (str == nullptr)
            return nullptr;

        char *p = str;
        while (*p != 0)
        {
            if (*p >= 'A' && *p <= 'Z')
                *p += 32;
            p++;
        }
        return str;
    }

    string &to_lower(string &str)
    {
        for (auto &cc : str)
        {
            if (cc >= 'A' && cc <= 'Z')
                cc += 32;
        }
        return str;
    }

    bool replace_str(char *str, const std::string &old_str, const std::string &new_str, const bool bloop)
    {
        // 检查输入有效性
        if (str == nullptr || old_str.empty())
            return false;

        // 若新字符串包含旧字符串且设置了循环替换，则返回false（防止无限循环）
        if (bloop && new_str.find(old_str) != std::string::npos)
            return false;

        size_t original_len = strlen(str);
        const char* old_str_ptr = old_str.c_str();
        const char* new_str_ptr = new_str.c_str();
        size_t old_str_len = old_str.length();
        size_t new_str_len = new_str.length();

        bool replaced = false;
        char* current_pos = str;
        
        while (true) {
            // 查找下一个匹配位置
            char* pos = strstr(current_pos, old_str_ptr);
            if (pos == nullptr)
                break; // 没有更多匹配
            
            // 计算替换后的新长度
            size_t before_len = pos - str;
            size_t after_len = original_len - before_len - old_str_len;
            size_t new_total_len = before_len + new_str_len + after_len;
            
            // 检查是否超出原始缓冲区大小
            if (new_total_len > original_len)
                return replaced; // 返回是否已经替换过任何内容
            
            // 创建临时缓冲区进行替换
            char* temp = new (std::nothrow) char[new_total_len + 1];
            if (temp == nullptr)
                return replaced;
            
            // 复制替换前的部分
            memcpy(temp, str, before_len);
            // 复制新字符串
            memcpy(temp + before_len, new_str_ptr, new_str_len);
            // 复制替换后的部分
            memcpy(temp + before_len + new_str_len, pos + old_str_len, after_len + 1); // +1 包含终止符
            
            // 将临时结果复制回原字符串
            memcpy(str, temp, new_total_len + 1);
            delete[] temp;
            
            original_len = new_total_len; // 更新长度
            replaced = true;
            
            if (bloop) {
                // 循环替换模式：从头开始重新查找
                current_pos = str;
            } else {
                // 单次替换模式：从替换位置之后继续查找
                current_pos = pos + new_str_len;
            }
        }
        
        return replaced;
    }

    bool replace_str(string &str, const string &old_str, const string &new_str, const bool bloop)
    {
        if (str.empty() || old_str.empty())
            return false;

        // 若新字符串包含旧字符串且设置了循环替换，则返回false
        if (bloop == true && (new_str.find(old_str) != string::npos))
            return false;

        int start_pos = 0;
        int cur_pos = 0;

        while (true)
        {
            if (bloop == true)
                cur_pos = str.find(old_str);
            else
                cur_pos = str.find(old_str, start_pos);

            if (cur_pos == string::npos)
                break;

            str.replace(cur_pos, old_str.size(), new_str);

            if (bloop == false)
                start_pos = cur_pos + new_str.size();
        }
        return true;
    }

    char *pick_number(const string &src, char *dest, const bool bsigned, const bool bdot)
    {
        if (dest == nullptr)
            return nullptr;
        
        string str_temp = pick_number(src, bsigned, bdot);
        str_temp.copy(dest, str_temp.size() + 1);           // string的copy()不会给C风格字符串末尾加\0
        dest[str_temp.size()] = '\0';

        return dest;
    }

    string& pick_number(const string &src, string &dest, const bool bsigned, const bool bdot)
    {
        // 避免src与dest指向同一块内存时，src被修改
        string str;

        for(char cc : src)
        {
            if((bsigned && (cc == '+' || cc == '-')))
            {
                str += cc;
                continue;
            }
            if((bdot && (cc == '.')))
            {
                str += cc;
                continue;
            }
            if(isdigit(cc))
                str += cc;
        }

        dest = str;

        return dest;
    }

    string pick_number(const string &src, const bool bsigned, const bool bdot)
    {
        string dest;
        pick_number(src, dest, bsigned, bdot);
        return dest;
    }

    bool match_str(const string &str, const string &rules)
    {
        // 规则为空时直接返回失败
        if(rules.empty()) 
            return false;

        // 通配符"*"匹配任意字符串
        if(rules == "*")
            return true;

        int ii, jj;
        int pos_1, pos_2;
        ccmdstr cmd_str, cmd_sub_str;

        // 转换为大写进行比较（不区分大小写匹配）
        string file_name = str;
        string match_str = rules;
        to_upper(file_name);
        to_upper(match_str);

        // 将规则按逗号分割为多个子规则
        cmd_str.split_to_cmd(match_str, ",");

        // 遍历每个子规则（逗号分隔的部分）
        for(ii = 0; ii < cmd_str.size(); ii++)
        {
            // 跳过空规则
            if(cmd_str[ii].empty())
                continue;

            pos_1 = pos_2 = 0;
            // 将子规则按星号分割为多个匹配片段
            cmd_sub_str.split_to_cmd(cmd_str[ii], "*");

            // 遍历每个匹配片段（星号分隔的部分）
            for(jj = 0; jj < cmd_sub_str.size(); jj++)
            {
                // 处理开头匹配：要求字符串起始部分与首个片段一致
                if(jj == 0)
                    if(file_name.substr(0, cmd_sub_str[jj].length()) != cmd_sub_str[jj])
                        break;

                // 处理结尾匹配：要求字符串结尾部分与最后一个片段一致
                if(jj == cmd_sub_str.size() - 1)
                    if(file_name.find(cmd_sub_str[jj], file_name.length() - cmd_sub_str[jj].length()) == string::npos)
                        break;

                // 处理中间匹配：在剩余部分查找当前片段
                pos_2 = file_name.find(cmd_sub_str[jj], pos_1);

                // 未找到匹配片段，退出循环
                if(pos_2 == string::npos)
                    break;

                // 更新搜索起始位置为当前匹配后的下一个位置
                pos_1 = pos_2 + cmd_sub_str[jj].length();
            }

            // 所有片段都匹配成功，返回true
            if(jj == cmd_sub_str.size())
                return true;
        }
        
        // 所有子规则都不匹配，返回false
        return false;
    }

    ccmdstr::ccmdstr(const string &buffer, const string sep_str, const bool b_del_space)
    {
        split_to_cmd(buffer, sep_str, b_del_space);
    }

    void ccmdstr::split_to_cmd(const string &buffer, const string &sep_str, const bool b_del_space)
    {
        m_cmd_str.clear(); 

        int pos_start = 0;
        int pos_next = 0;
        string sub_str;

        while((pos_next = buffer.find(sep_str, pos_start)) != string::npos)
        {
            sub_str = buffer.substr(pos_start, pos_next - pos_start);

            if(b_del_space)
                delete_lrchr(sub_str);

            m_cmd_str.push_back(std::move(sub_str));

            pos_start = pos_next + sep_str.size();
        }

        // 处理最后一个字段
        sub_str = buffer.substr(pos_start);
        if(b_del_space)
            delete_lrchr(sub_str);
        m_cmd_str.push_back(std::move(sub_str));
    }

    bool ccmdstr::getvalue(const int ii,string &value,const int i_len) const
    {
        if(ii>m_cmd_str.size())
            return false;

        int i_tmp_len = m_cmd_str[ii].size();
        if(i_len>0 && i_len<i_tmp_len)
            i_tmp_len = i_len;
        value = m_cmd_str[ii].substr(0,i_tmp_len);

        return true;
    }

    bool ccmdstr::getvalue(const int ii,char *value,const int i_len) const
    {
        if(ii>m_cmd_str.size() || value == nullptr)
            return false;

        if(i_len > 0)
            memset(value,0,i_len + 1);

        if(m_cmd_str[ii].size()<=(unsigned int)i_len || i_len == 0)
        {
            m_cmd_str[ii].copy(value,m_cmd_str[ii].size());
            value[m_cmd_str[ii].size()] = '\0';
        }
        else
        {
            m_cmd_str[ii].copy(value,i_len);
            value[i_len] = '\0';
        }

        return true;
    }
    
    bool ccmdstr::getvalue(const int ii,int &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stoi(pick_number(m_cmd_str[ii], true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
    }

    bool ccmdstr::getvalue(const int ii,unsigned int &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stoi(pick_number(m_cmd_str[ii]));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
        
    }

    bool ccmdstr::getvalue(const int ii,long &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stol(pick_number(m_cmd_str[ii], true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
        
    }
    
    bool ccmdstr::getvalue(const int ii,unsigned long &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stoul(pick_number(m_cmd_str[ii]));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
        
    }
    
    bool ccmdstr::getvalue(const int ii,double &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stod(pick_number(m_cmd_str[ii], true, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
        
    }
    
    bool ccmdstr::getvalue(const int ii,float &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        try
        {
            value = stof(pick_number(m_cmd_str[ii], true, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;
        
    }
    
    bool ccmdstr::getvalue(const int ii,bool &value) const
    {
        if(ii >= m_cmd_str.size())
            return false;

        string str = m_cmd_str[ii];
        to_upper(str);

        if(str == "TRUE")
            value = true;
        else
            value = false;
        return true;
    }

    ccmdstr::~ccmdstr()
    {
        m_cmd_str.clear();
    }

    ostream& operator<<(ostream& os, const ccmdstr& cmdstr)
    {
        for(int i = 0; i < cmdstr.size(); i++)
        {
            os << "[" << i << "] = " << cmdstr[i] << endl;
        }
        return os;
    }

    bool get_xml_buffer(const string &xml_buffer, const string &field_name, string &value, int i_len)
    {
        string start = "<" + field_name + ">";
        string end = "</" + field_name + ">";

        int start_pos = xml_buffer.find(start);
        if(start_pos == string::npos)
            return false;

        int end_pos = xml_buffer.find(end, start_pos + start.size());

        int i_tmp_len = end_pos - start_pos - start.size();
        // 若需要截取长度，则取两者中较小的长度
        if(i_len > 0 && i_tmp_len > i_len)
            i_tmp_len = i_len;

        value = xml_buffer.substr(start_pos + start.size(), i_tmp_len);

        return true;
    }
    bool get_xml_buffer(const string &xml_buffer, const string &field_name, char* value, int i_len)
    {
        if(value == nullptr)
            return false;

        if(i_len > 0)
            memset(value, 0, i_len + 1);

        string str;
        get_xml_buffer(xml_buffer, field_name, str);

        // 若不需要截断字符串
        if(str.size() <= (unsigned int)i_len || i_len == 0)
        {
            str.copy(value, str.size());
            value[str.size()] = '\0';
        }
        else
        {
            str.copy(value, i_len);
            value[i_len] = '\0';
        }
        return true;
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,bool &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;
        
        to_upper(str);

        if(str == "TRUE")
            value = true;
        else
            value = false;
        return true;
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,int &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stoi(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,unsigned int &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stoi(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,long &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stol(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,unsigned long &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stoul(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,double &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stod(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    bool get_xml_buffer(const string &xml_buffer,const string &field_name,float &value)
    {
        string str;
        if(!get_xml_buffer(xml_buffer, field_name, str))
            return false;

        try
        {
            value = stof(pick_number(str, true));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        return true;        
    }

    string& l_time(string& str_time, const string& fmt, const int time_tvl)
    {
        time_t timer;
        time(&timer);       // 获取系统当前时间

        timer += time_tvl;  // 时间偏移

        time_to_str(timer, str_time, fmt);

        return str_time;
    }

    char* l_time(char *str_time, const string& fmt, const int time_tvl)
    {
        if(str_time == nullptr)
            return nullptr;

        time_t timer;
        time(&timer);

        timer += time_tvl;

        time_to_str(timer, str_time, fmt);

        return str_time;
    }

    string l_time_1(const string& fmt, const int time_tvl)
    {
        string str_time;
        l_time(str_time, fmt, time_tvl);
        return str_time;
    }

    string& time_to_str(const time_t t_time, string &str_time, const string& fmt)
    {
        struct tm st_tm;
        // 将t_time转换为st_tm结构体（本地时间）
        localtime_r(&t_time, &st_tm); // 线程安全
        st_tm.tm_year += 1900;          // 年份从1900年开始
        st_tm.tm_mon++;                 // 月份从0开始

        if(fmt == "" || fmt == "yyyy-mm-dd hh24:mi:ss")
        {
            str_time = s_format("%04u-%02u-%02u %02u:%02u:%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour, st_tm.tm_min, st_tm.tm_sec);
        }
        else if(fmt == "yyyy-mm-dd hh24:mi")
        {
            str_time = s_format("%04u-%02u-%02u %02u:%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour, st_tm.tm_min);
        }
        else if(fmt == "yyyy-mm-dd hh24")
        {
            str_time = s_format("%04u-%02u-%02u %02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour);
        }
        else if(fmt == "yyyy-mm-dd")
        {
            str_time = s_format("%04u-%02u-%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday);
        }
        else if(fmt == "yyyy-mm")
        {
            str_time = s_format("%04u-%02u", st_tm.tm_year, st_tm.tm_mon);
        }
        else if(fmt == "yyyymmddhh24miss")
        {
            str_time = s_format("%04u%02u%02u%02u%02u%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour, st_tm.tm_min, st_tm.tm_sec);
        }
        else if(fmt == "yyyymmddhh24mi")
        {
            str_time = s_format("%04u%02u%02u%02u%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour, st_tm.tm_min);
        }
        else if(fmt == "yyyymmddhh24")
        {
            str_time = s_format("%04u%02u%02u%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday, st_tm.tm_hour);
        }
        else if(fmt == "yyyymmdd")
        {
            str_time = s_format("%04u%02u%02u", st_tm.tm_year, st_tm.tm_mon, st_tm.tm_mday);
        }
        else if(fmt == "hh24miss")
        {
            str_time = s_format("%02u%02u%02u", st_tm.tm_hour, st_tm.tm_min, st_tm.tm_sec);
        }
        else if(fmt == "hh24mi")
        {
            str_time = s_format("%02u%02u", st_tm.tm_hour, st_tm.tm_min);
        }
        else if(fmt == "hh24")
        {
            str_time = s_format("%02u", st_tm.tm_hour);
        }
        else if(fmt == "mi")
        {
            str_time = s_format("%02u", st_tm.tm_min);
        }
        return str_time;
    }

    char* time_to_str(const time_t t_time, char *str_time, const string& fmt)
    {
        if(str_time == nullptr)
            return nullptr;

        string str;
        time_to_str(t_time, str, fmt);
        str.copy(str_time, str.length());
        str_time[str.length()] = '\0';
        return str_time;
    }

    string time_to_str_1(const time_t t_time, const string& fmt)
    {
        string str;
        time_to_str(t_time, str, fmt);
        return str;
    }

    time_t str_to_time(const string& str_time)
    {
        string str_tmp;
        string yyyy, mm, dd, hh, mi, ss;

        pick_number(str_time, str_tmp, false, false);

        if(str_tmp.length() != 14)
            return -1;

        yyyy = str_tmp.substr(0, 4);
        mm = str_tmp.substr(4, 2);
        dd = str_tmp.substr(6, 2);
        hh = str_tmp.substr(8, 2);
        mi = str_tmp.substr(10, 2);
        ss = str_tmp.substr(12, 2);

        struct tm st_tm;

        try
        {
            st_tm.tm_year = stoi(yyyy) - 1900;
            st_tm.tm_mon = stoi(mm) - 1;
            st_tm.tm_mday = stoi(dd);
            st_tm.tm_hour = stoi(hh);
            st_tm.tm_min = stoi(mi);
            st_tm.tm_sec = stoi(ss);
            // 设置不使用夏令时
            st_tm.tm_isdst = 0;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return -1;
        }
        // 转换成时间戳并返回
        return mktime(&st_tm);
    }

    bool add_time(const string &in_time, char* out_time, const int time_tvl, const string &fmt)
    {
        if(out_time == nullptr)
            return false;
        
        time_t timer;

        if((timer = str_to_time(in_time)) == -1)
        {
            strcpy(out_time, "");
            return false;
        }

        timer += time_tvl;

        time_to_str(timer, out_time, fmt);

        return true;
    }

    bool add_time(const string &in_time, string &out_time, const int time_tvl, const string &fmt)
    {
        time_t timer;

        // 将字符串转换成时间戳
        if((timer = str_to_time(in_time)) == -1)
        {
            out_time = "";
            return false;
        }

        timer += time_tvl;

        time_to_str(timer, out_time, fmt);

        return true;
    }

    ctimer::ctimer()
    {
        start();
    }

    void ctimer::start()
    {
        memset(&m_start, 0, sizeof(struct timeval));
        memset(&m_end, 0, sizeof(struct timeval));

        gettimeofday(&m_start, 0); // 获取当前时间，精确到微妙
    }

    double ctimer::elapsed()
    {
        gettimeofday(&m_end, NULL);

        // 直接计算时间差（秒 + 微秒/1e6），避免字符串转换
        double elapsed_sec = (m_end.tv_sec - m_start.tv_sec) + 
                            (m_end.tv_usec - m_start.tv_usec) / 1000000.0;

        start();  // 重置计时器

        return elapsed_sec;
    }

    bool new_dir(const string &path_or_filename, bool b_is_filename)
    {
        // 查找的初始索引，0为根目录'/'，因此从1开始
        int pos = 1;

        while (true)
        {
            int pos_1 = path_or_filename.find("/", pos);

            if(pos_1 == string::npos)
                 break;

            // 获取目录名
            string str_path_name = path_or_filename.substr(0, pos_1);

            // 更新下次查找位置
            pos = pos_1 + 1;

            // 若目录不存在
            if(access(str_path_name.c_str(), F_OK) != 0)
            {   
                // 若创建失败
                if(mkdir(str_path_name.c_str(), 0755) != 0)
                    return false;
            }
        }

        // 若不是文件，最后一个分隔符的目录也需要创建
        if(b_is_filename == false)
        {
            if(access(path_or_filename.c_str(), F_OK) != 0)
            {
                if(mkdir(path_or_filename.c_str(), 0755) != 0)
                    return false;

            }
        }
        return true;
    }

    bool rename_file(const string &src_filename, const string &dst_filename)
    {
        if(access(src_filename.c_str(), F_OK) != 0)
            return false;

        if(new_dir(dst_filename, true) == false)
            return false;

        // 调用库函数重命名文件
        if(rename(src_filename.c_str(), dst_filename.c_str()) == 0)
            return true;
        else
            return false;
    }

    bool copy_file(const string &src_filename, const string &dst_filename)
    {
        if(new_dir(dst_filename, true) == false)
            return false;

        cifile i_file;
        cofile o_file;
        int i_file_size = get_file_size(src_filename);

        int total_bytes = 0;
        int cur_read = 0;
        char buffer[5000];

        if(i_file.open_file(src_filename, ios::in | ios::binary) == false)
            return false;

        if(o_file.open_file(dst_filename, ios::out | ios::binary) == false)
            return false;

        while (true)
        {
            if(i_file_size - total_bytes > 5000)
                cur_read = 5000;
            else
                cur_read = i_file_size - total_bytes;

            memset(buffer, 0, sizeof(buffer));
            i_file.read(buffer, cur_read);
            o_file.write(buffer, cur_read);

            total_bytes += cur_read;

            if(total_bytes >= i_file_size)
                break;
        }

        i_file.close();
        o_file.close_and_rename();

        // 更改文件的修改时间属性
        string str_mtime;
        file_mtime(src_filename, str_mtime);
        set_mtime(dst_filename, str_mtime);
        
        return true;
    }

    int get_file_size(const string &filename)
    {
        // 存放文件信息的结构体
        struct stat st_file_stat;

        // 获取文件信息并保存在结构体中
        if(stat(filename.c_str(), &st_file_stat) < 0)
            return -1;

        return st_file_stat.st_size;
    }

    bool file_mtime(const string &filename, char *mtime, const string &fmt)
    {
        struct stat st_file_stat;

        if(stat(filename.c_str(), &st_file_stat) < 0)
            return false;

        time_to_str(st_file_stat.st_mtime, mtime, fmt);

        return true;
    }

    bool file_mtime(const string &filename, string &mtime, const string &fmt)
    {
        struct stat st_file_stat;

        if(stat(filename.c_str(), &st_file_stat) < 0)
            return false;

        time_to_str(st_file_stat.st_mtime, mtime, fmt);

        return true;
    }

    bool set_mtime(const string &filename, const string &mtime)
    {
        // 存放文件时间的结构体
        struct utimbuf stu_timebuf;

        stu_timebuf.actime = stu_timebuf.modtime = str_to_time(mtime);

        // 设置文件时间
        if(utime(filename.c_str(), &stu_timebuf) != 0)
            return false;

        return true;
    }

    void cdir::set_fmt(const string &fmt)
    {
        m_fmt = fmt;
    }

    bool cdir::open_dir(const string &dir_name, const string &rules, const int max_files, const bool band_child, bool b_sort)
    {
        // 清空文件列表
        m_filelist.clear();
        m_pos = 0;

        // 若目录不存在，则创建
        if(new_dir(dir_name, false) == false)
        {
            return false;
        }

        // 打开目录，获取目录中的文件列表，存放在m_filelist中
        bool b_ret = _open_dir(dir_name, rules, max_files, band_child);

        if(b_sort == true)
            sort(m_filelist.begin(), m_filelist.end());

        return b_ret;
    }

    bool cdir::_open_dir(const string &dir_name, const string &rules, const int max_files, const bool band_child)
    {
        DIR *dir;

        // 打开目录
        if((dir = opendir(dir_name.c_str())) == nullptr)
            return false;

         string str_ffilename;          // 全路径文件名
         struct dirent *st_dir;         // 存放从目录中读取的内容

         while((st_dir = readdir(dir)) != 0)
         {
            if(m_filelist.size() >= max_files)
                break;

            // 忽略上一级目录及以'.'开头的特殊目录和文件
            if(st_dir->d_name[0] == '.')
                continue;

            // 拼接全路径的文件名
            str_ffilename = dir_name + "/" + st_dir->d_name;

            // 若是目录，处理各级子目录
            if(st_dir->d_type == 4)
            {
                // 若递归处理子目录
                if(band_child == true)
                {
                    if(_open_dir(str_ffilename, rules, max_files, band_child) == false)
                    {
                        closedir(dir);
                        return false;
                    }
                }
            }
            // 若是普通文件
            else if(st_dir->d_type == 8)
            {
                if(match_str(str_ffilename, rules) == false)
                    continue;

                m_filelist.push_back(std::move(str_ffilename));
            }
         }
        closedir(dir);
        return true;
    }

    bool cdir::read_dir()
    {
        // printf("----------m_pos = %d, m_filelist.size() = %d----------\n", m_pos, m_filelist.size());
        if(m_pos >= m_filelist.size())
        {
            m_pos = 0;
            m_filelist.clear();
            return false;
        }

        // 获取文件全名，包括路径
        m_ffilename = m_filelist[m_pos];
        
        // 从绝对路径的文件名中解析目录名和文件名
        int pp = m_ffilename.find_last_of("/");
        m_dirname = m_ffilename.substr(0, pp);
        m_filename = m_ffilename.substr(pp + 1);

        // 获取文件信息
        struct stat st_filestat;
        stat(m_ffilename.c_str(), &st_filestat);
        m_filesize = st_filestat.st_size;
        m_mtime = time_to_str_1(st_filestat.st_mtime, m_fmt);
        m_ctime = time_to_str_1(st_filestat.st_ctime, m_fmt);
        m_atime = time_to_str_1(st_filestat.st_atime, m_fmt);

        m_pos++;

        return true;
    }

    cdir::~cdir()
    {
        m_pos = 0;
        m_filelist.clear();
    }

    bool cofile::open_file(const string &filename, const bool b_tmp, const ios::openmode mode, const bool ben_buffer)
    {
        // 若文件当前为打开状态，则先关闭
        if(f_out.is_open())
            f_out.close();

        m_filename = filename;

        // 若文件的目录不存在，则创建目录
        new_dir(m_filename, true);

        if(b_tmp == true)
        {
            m_filename_tmp += ".tmp";
            f_out.open(m_filename_tmp, mode);
        }
        else
        {
            m_filename_tmp.clear();
            f_out.open(m_filename, mode);
        }

        // 若不采用缓冲区，则开启立即刷新模式
        if(ben_buffer == false)
            f_out << unitbuf;

        return f_out.is_open();
    }

    bool cofile::write(void *buf, int buf_size)
    {
        if(f_out.is_open() == false)
            return false;

        // 将任意类型的数据以二进制方式写入文件
        f_out.write(static_cast<char*>(buf), buf_size);

        return f_out.good(); 
    }

    bool cofile::close_and_rename()
    {
        if(f_out.is_open() == false)
            return false;

        f_out.close();

        if(m_filename_tmp.empty() == false) 
            if(rename_file(m_filename_tmp, m_filename) == false)
                return false;

        return true;
    }

    void cofile::close()
    {
        if(f_out.is_open())
            f_out.close();
        else
            return;
    }

    bool clogfile::clear()
    {
        if (!f_out.is_open())
            return false;

        const std::string filename = m_filename;
        f_out.close(); // 关闭输出流

        // 直接以截断模式重新打开输出流（清空内容）
        f_out.open(filename, std::ios::trunc);
        return f_out.is_open(); // 返回打开操作的结果
    }

    bool cifile::open_file(const string &filename, const ios::openmode mode)
    {
        if(f_in.is_open())
            f_in.close();

        m_filename = filename;

        f_in.open(filename, mode);

        return f_in.is_open();
    }

    int cifile::read(void *buf, int buf_size)
    {
        // 从文件中读取buf_size个字节到buf中
        f_in.read(static_cast<char*>(buf), buf_size);

        // 返回读取的字节数
        return f_in.gcount();
    }

    char* cifile::read_all()
    {
        // 获取文件大小
        f_in.seekg(0, std::ios::end);
        const std::streamsize file_size = f_in.tellg();
        f_in.seekg(0, std::ios::beg);

        char *buf = new char[file_size + 1];

        f_in.read(static_cast<char*>(buf), file_size);

        return buf;
    }

    bool cifile::close_and_remove()
    {
        if(f_in.is_open() == false)
            return false;

        f_in.close();

        if(remove(m_filename.c_str()) != 0)
            return false;

        return true;
    }

    void cifile::close()
    {
        if(f_in.is_open() == false)
            return;

        f_in.close();
    }

    bool cifile::read_line(string &buf, const string &end_sign)
    {
        buf.clear();
        string str_line;

        while (getline(f_in, str_line))
        {
            buf += str_line;

            if (end_sign == "")
            {
                // 如果没有指定结束符，读取单行（物理行）即成功返回。
                return true;
            }
            else
            {
                // 检查缓冲区是否以指定的 end_sign 结尾。
                if (buf.length() >= end_sign.length() &&
                    buf.compare(buf.length() - end_sign.length(), end_sign.length(), end_sign) == 0)
                {
                    // 找到了结束标志，成功返回。
                    return true;
                }
            }

            // 如果还没找到 end_sign，说明正在读取一个逻辑上的“多行块”，
            // 需要继续读取文件的下一行。此时，我们将 getline “消耗”掉的换行符加回来，
            // 以保持多行内容的原始格式。
            buf += "\n";
        }

        // 当循环因为到达文件末尾而结束时，如果 buf 中有任何内容，
        // 说明我们成功读取了文件的最后一部分数据。此时应该返回 true，
        // 以确保调用方能够处理这最后的数据。
        // 如果文件从一开始就是空的，或者已经读完，buf 会为空，返回 false。
        return !buf.empty();
    }

    bool clogfile::open(const string &filename, const ios::openmode mode, const bool b_backup, bool ben_buffer)
    {
        if(f_out.is_open())
            return false;

        m_filename = filename;
        m_mode = mode;
        m_backup = b_backup;
        m_ben_buffer = ben_buffer;

        new_dir(filename, true);

        f_out.open(filename, mode);

        if(m_ben_buffer == false)
            f_out << unitbuf;

        return f_out.is_open();
    }

    bool clogfile::backup()
    {
        if(m_backup == false)
            return true;

        if(f_out.is_open() == false)
            return false;

        if(f_out.tellp() > m_maxsize)
        {
            m_splock.lock();

            f_out.close();

            string backup_filename = m_filename + "." + l_time_1("yyyymmddhh24miss");
            rename_file(m_filename, backup_filename);

            f_out.open(m_filename, m_mode);

            if(m_ben_buffer == false)
                f_out << unitbuf;

            m_splock.unlock();

            return f_out.is_open();
        }
        return true;
    }

    bool ctcpclient::connect(const string &ip, const int port)
    {
        // 若已连接到服务器，则断开
        if(m_client_fd != -1)
        {
            close(m_client_fd);
            m_client_fd = -1;
        }

        // 忽略SIGPIPE信号，防止程序异常退出
        signal(SIGPIPE, SIG_IGN);

        m_ip = ip;
        m_port = port;

        struct hostent *host;       // 主机信息结构体
        struct sockaddr_in serv_addr; // 服务器地址结构体

        // 建立socket
        if((m_client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            return false;

        // 若域名解析失败
        if(!(host = gethostbyname(m_ip.c_str())))
        {
            ::close(m_client_fd);
            m_client_fd = -1;
            return false;
        }

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(m_port);
        memcpy(&serv_addr.sin_addr, host->h_addr, host->h_length);

        // 连接服务器
        if(::connect(m_client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
        {
            close(m_client_fd);
            m_client_fd = -1;
            return false;
        }

        return true;
    }

    bool ctcpclient::read(void *buffer, const int i_buffer_size, const int i_out_time)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_read(m_client_fd, buffer, i_buffer_size, i_out_time);
    }

    bool ctcpclient::read(string &buffer, const int i_out_time)
    {
        if(m_client_fd == -1) 
            return false;

        return tcp_read(m_client_fd, buffer, i_out_time);
    }

    bool ctcpclient::write(const void *buffer, const int i_buffer_size)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_write(m_client_fd, (char *)buffer, i_buffer_size);
    }

    bool ctcpclient::write(const string &buffer)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_write(m_client_fd, buffer);
    }

    void ctcpclient::close_connect()
    {
        if(m_client_fd >= 0)
            close(m_client_fd);

        m_client_fd = -1;
        m_port = 0;
    }

    ctcpclient::~ctcpclient()
    {
        close_connect();
    }

    bool ctcpserver::init_server(const unsigned int port, const int back_log)
    {
        if(m_listen_fd > 0)
        {
            close(m_listen_fd);
            m_listen_fd = -1;
        }

        if((m_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
            return false;

        // 忽略SIFPIPE信号，防止程序异常退出
        signal(SIGPIPE, SIG_IGN);

        // 打开SO_REUSEADDR选项，当服务端处于TIME_WAIT状态时，允许服务端快速重启
        int reuse = 1;
        setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        memset(&m_server_addr, 0, sizeof(m_server_addr));
        m_server_addr.sin_family = AF_INET;
        m_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        m_server_addr.sin_port = htons(port);
        // 将套接字绑定到地址结构
        if(bind(m_listen_fd, (struct sockaddr *)&m_server_addr, sizeof(m_server_addr)) != 0)
        {
            close_listen();
            return false;
        }

        // 监听套接字
        if(listen(m_listen_fd, back_log) != 0)
        {
            close_listen();
            return false;
        }

        return true;
    }

    bool ctcpserver::accept_client()
    {
        if(m_listen_fd == -1)
            return false;

        int m_socket_addr_len = sizeof(struct sockaddr_in); 
        if((m_client_fd = accept(m_listen_fd, (struct sockaddr *)&m_client_addr, (socklen_t *)&m_socket_addr_len)) < 0)
            return false;

        return true;
    }

    char *ctcpserver::get_ip()
    {
        return inet_ntoa(m_client_addr.sin_addr);
    }

    bool ctcpserver::read(string &buffer, const int i_out_time)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_read(m_client_fd, buffer, i_out_time);
    }

    bool ctcpserver::read(void *buffer, const int i_buffer_len, const int i_out_time)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_read(m_client_fd, buffer, i_buffer_len, i_out_time);
    }

    bool ctcpserver::write(const string &buffer)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_write(m_client_fd, buffer);
    }

    bool ctcpserver::write(const void *buffer, const int i_buffer_len)
    {
        if(m_client_fd == -1)
            return false;

        return tcp_write(m_client_fd, buffer, i_buffer_len);
    }

    void ctcpserver::close_listen()
    {
        if(m_listen_fd >= 0)
        {
            close(m_listen_fd);
            m_listen_fd = -1;
        }
    }

    void ctcpserver::close_client()
    {
        if(m_client_fd >= 0)
        {
            close(m_client_fd);
            m_client_fd = -1;
        }
    }

    ctcpserver::~ctcpserver()
    {
        close_client();
        close_listen();
    }

    bool tcp_read(const int socket_fd, void *buffer, const int i_buffer_size,const int i_out_time)
    {
        if(socket_fd == -1)
            return false;

        if(i_out_time > 0)
        {
            struct pollfd fds;
            fds.fd = socket_fd;                 // 要监听的套接字
            fds.events = POLLIN;                // 表示关注可读事件（如接收数据，连接关闭）
            if(poll(&fds, 1, i_out_time * 1000) <= 0)  // 等待事件或超时
                return false;
        }

        // 不等待
        if(i_out_time == -1)
        {
            struct pollfd fds;
            fds.fd = socket_fd;
            fds.events = POLLIN;
            if(poll(&fds, 1, 0) <= 0)
                return false;
        }

        if(read_n(socket_fd, (char *)buffer, i_buffer_size) == false)
            return false;

        return true;
    }

    bool tcp_read(const int socket_fd, string &buffer, const int i_out_time)
    {
        if(socket_fd == -1)
            return false;

        if(i_out_time > 0)
        {
            struct pollfd fds;
            fds.fd = socket_fd;
            fds.events = POLLIN;
            if(poll(&fds, 1, i_out_time * 1000) <= 0)
                return false;
        }

        if(i_out_time == -1)
        {
            struct pollfd fds;
            fds.fd = socket_fd;
            fds.events = POLLIN;
            while(poll(&fds, 1, 0) <= 0)
                return false;
        }

        int buffer_size = 0;

        // 读取该报文的数据长度（四个字节）
        if(read_n(socket_fd, (char*)&buffer_size, 4) == false)
            return false;

        buffer.resize(buffer_size);

        if(read_n(socket_fd, &buffer[0], buffer_size) == false)
            return false;

        return true;
    }

    bool tcp_write(const int socket_fd, const void *buffer, const int i_buffer_size)
    {
        if(socket_fd == -1)
            return false;

        if(write_n(socket_fd, (char*)buffer, i_buffer_size) == false)
            return false;

        return true;
    }

    bool tcp_write(const int socket_fd, const string &buffer)
    {
        if(socket_fd == -1)
            return false;

        int buffer_size = buffer.size();

        // 先发送报头
        if(write_n(socket_fd, (char*)&buffer_size, 4) == false)
            return false;

        // 在发送报文体
        if(write_n(socket_fd, buffer.c_str(), buffer_size) == false)
            return false;

        return true;
    }

    bool read_n(const int socket_fd, char *buffer, const size_t n)
    {
        int n_left = n;     // 剩余需要读取的字节数
        int idx = 0;        // 已读取的字节数
        int n_cur_readed;     // 每次调用recv()实际读取的字节数

        while (n_left > 0)
        {
            if((n_cur_readed = recv(socket_fd, buffer + idx, n_left, 0)) <= 0)
                return false;

            idx += n_cur_readed;
            n_left -= n_cur_readed;
        }
        return true;
    }

    bool write_n(const int socket_fd, const char *buffer, const size_t n)
    {
        int n_left = n;
        int idx = 0;
        int n_cur_wrirren;

        while (n_left > 0)
        {
            if((n_cur_wrirren = send(socket_fd, buffer + idx, n_left, 0)) <= 0)
                return false;

            idx += n_cur_wrirren;
            n_left -= n_cur_wrirren;
        }
        return true;
    }
    
    void close_io_and_signal(bool b_close_io)
    {
        for(int i = 0; i < 64; i++)
        {
            if(b_close_io == true)
                close(i);
            signal(i, SIG_IGN);
        } 
    }

    bool csemp::init(key_t key, unsigned short value, short sem_flg)
    {
        // 若已经初始化
        if(m_sem_id != -1)
            return false;

        // 保留权限标志
        m_sem_flg = sem_flg;

        // 尝试获取已存在的信号量
        if((m_sem_id = semget(key, 1, 0666)) == -1)
        {
            // 若信号量不存在
            if(errno == ENOENT)
            {
                // 创建并初始化信号量
                // IPC_EXCL标志确保只有一个进程可以创建信号量
                if((m_sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1)
                {
                    // 上一步操作非原子操作，还需进行判断，避免创建冲突
                    if(errno == EEXIST)
                    {
                        if((m_sem_id = semget(key, 1, 0666)) == -1)
                        {
                            perror("init 1 semget error");
                            return false;
                        }
                        return true;
                    }
                    else
                    {
                        perror("init 2 semget error");
                        return false;
                    }
                }

                // 初始化信号量值
                union semun sem_union;
                sem_union.val = value;
                // 将信号量(索引0)的值设置为value（通过sem_union.val传递）
                if(semctl(m_sem_id, 0, SETVAL, sem_union) < 0)
                {
                    perror("init semctl error");
                    return false;
                }
            }
            else
            {
                perror("init 3 semget error");
                return false;
            }
        }
        return true;
    }
    bool csemp::wait(short value)
    {
        /**
         * 检查信号量是否已经初始化
         * 若未初始化，直接返回false
         */
        if(m_sem_id == -1)
            return false;

        /**
         * 构造sembuf结构体
         * sem_num: 信号量编号
         * sem_op: 信号量操作值
         * sem_flg: 信号量标志位
         */
        struct sembuf sem_buf;
        sem_buf.sem_num = 0;
        sem_buf.sem_op = value;
        sem_buf.sem_flg = m_sem_flg;

        /**
         * 执行semop系统调用
         * 对信号量进行操作
         */
        if(semop(m_sem_id, &sem_buf, 1) == -1)
        {
            perror("semop wait error");
            return false;
        }

        return true;
    }
    bool csemp::post(short value)
    {
        if(m_sem_id == -1)
        {
            return false;
        } 

        struct sembuf sem_buf;
        sem_buf.sem_num = 0;
        sem_buf.sem_op = value;
        sem_buf.sem_flg = m_sem_flg;

        if(semop(m_sem_id, &sem_buf, 1) == -1)
        {
            perror("semop post error");
            return false;
        }
        return true;
    }
    int csemp::get_value()
    {
        /**
         * 使用semctl系统调用获取信号量的当前值
         */
        return semctl(m_sem_id, 0, GETVAL); 
    }

    int csemp::get_m_sem_id()
    {
        return m_sem_id;
    }

    bool csemp::destroy()
    {
        if(m_sem_id == -1)
        {
            return true;
        } 

        if(semctl(m_sem_id, 0, IPC_RMID) == -1)
        {
            perror("semctl destroy error");
            return false;
        }
        return true;
    }

    csemp::~csemp()
    {
        // destroy();
    }

    cpactive::cpactive()
    {
        m_shm_id = 0;
        m_pos = -1;
        m_shm = 0;
    }

    bool cpactive::add_p_info(const int out_time, const string &p_name, clogfile * log_file)
    {
        if(m_pos != -1)
            return true;

        // 创建/获取共享内存，键值为SHMKEYP，大小为MAXNUMP * sizeof(st_proc_info)
        if((m_shm_id = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(st_proc_info), 0666 | IPC_CREAT)) == -1)
        {
            if(log_file != nullptr)
                log_file->write("shmget: %x error\n", SHMKEYP);
            else
                cout << "shmget: " << SHMKEYP << " error" << endl;
            return false;
        }

        // 将共享空间连接到当前进程的地址空间
        m_shm = (struct st_proc_info *)shmat(m_shm_id, 0, 0);

        // 当前进程心跳信息
        st_proc_info proc_info(getpid(), p_name.c_str(), out_time, time(0));

        // 若共享内存中已存在当前进程编号，说明是其他进程的残留信息，当前进程应该重用该位置
        for(int i = 0; i < MAXNUMP; i++)
        {
            if((m_shm + i)->pid == proc_info.pid)
            {
                m_pos = i;
                break;
            }
        }

        csemp sem;
        if(sem.init(SEMKEYP) == false)
        {
            if(log_file != nullptr)
                log_file->write("init sem %x error.\n", SEMKEYP);
            else
                cerr << "init sem " << SEMKEYP << " error." << endl;

            return false;
        }

        sem.wait();
        // 若共享内存的进程组中不存在当前进程的编号，找一个空位置
        if(m_pos == -1)
        {
            for(int i = 0; i < MAXNUMP; i++)
                if((m_shm + i)->pid == 0)
                {
                    m_pos = i;
                    break;
                }
        }

        if(m_pos == -1)
        {
            if(log_file != nullptr)
                log_file->write("shared memory fulled, no more position.\n");
            else
                cout << "shared memory fulled, no more position." << endl;

            sem.post();
            return false;
        }

        memcpy(m_shm + m_pos, &proc_info, sizeof(st_proc_info));

        sem.post();

        return true;
    }

    bool cpactive::upt_atime()
    {
        if(m_pos == -1) 
            return false;
        
        (m_shm + m_pos)->a_time = time(nullptr);

        return true;
    }

    cpactive::~cpactive()
    {
        if(m_pos != -1)
            memset(m_shm + m_pos, 0, sizeof(st_proc_info));

        // 将共享内存从当前进程中分离
        if(m_shm != 0)
            shmdt(m_shm);
    }
}