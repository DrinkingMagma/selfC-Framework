# 启动进程守护模块
/root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/tools/bin/checkproc /root/C++/selfC++Framework/log/checkproc.log

# 每分钟生成一次数据
/root/C++/selfC++Framework/tools/bin/procctl 10 /root/C++/selfC++Framework/idc/bin/crtsurfdata /root/C++/selfC++Framework/idc/ini/stcode.ini /root/C++/selfC++Framework/idc/output/surfdata/ /root/C++/selfC++Framework/idc/log/crusurfdata.log csv,json,xml