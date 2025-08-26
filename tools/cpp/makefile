CXX := g++
INCLUDEDIR := -I../../public/ -I../../public/db/ -I$(ORACLE_HOME)/rdbms/public/
CFLAGS = -g -Wall -Wno-write-strings
LIBDIR := -L../../public/ -L../../public/db/ -L$(ORACLE_HOME)/lib/
LIBS := -l_public -l_ftp -l_ooci -lclntsh

# 搜集所有cpp文件
TEST_SRCS := $(wildcard *.cpp)
# 生成对应的可执行文件列表
TEST_BINS := $(TEST_SRCS:.cpp=)

all:$(TEST_BINS)

%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

# 清理目标
clean:
	rm -f $(TEST_BINS)