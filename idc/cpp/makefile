CXX := g++
INCLUDEDIR := -I../../public/ -I../../public/db/ -I$(ORACLE_HOME)/rdbms/public/
LIBDIR := -L../../public/ -L../../public/db/ -L$(ORACLE_HOME)/lib/
LIBS := -l_public -l_ooci -lclntsh
CFLAGS = -g -Wall -Wno-write-strings

# 搜集所有cpp文件
SRCS := $(wildcard *.cpp)
# 过滤掉 idcapp.cpp
SRCS := $(filter-out idcapp.cpp, $(SRCS))
# 生成对应的可执行文件列表
BINS := $(SRCS:.cpp=)

all:$(BINS)

# 为botmindtodb.cpp添加特殊编译规则：包含idcapp.cpp
obtmindtodb: obtmindtodb.cpp idcapp.h idcapp.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

# 清理目标
clean:
	rm -f $(BINS)