CXX := g++
INCLUDEDIR := -I../../public/ -I../../public/db/ -I$(ORACLE_HOME)/rdbms/public/
CFLAGS = -g -Wall -Wno-write-strings
LIBDIR := -L../../public/ -L../../public/db/ -L$(ORACLE_HOME)/lib/
LIBS := -l_public -l_ftp -l_ooci -lclntsh

# 搜集所有cpp文件
SRCS := $(wildcard *.cpp)
SRCS := $(filter-out _tools.cpp, $(SRCS))

# 生成对应的可执行文件列表
BINS := $(SRCS:.cpp=)

all:$(BINS)

xmltodb: xmltodb.cpp _tools.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

migratetable: migratetable.cpp _tools.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

syncref: syncref.cpp _tools.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

syncinc: syncinc.cpp _tools.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

# 清理目标
clean:
	rm -f $(BINS)