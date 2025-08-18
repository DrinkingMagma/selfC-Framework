# oracle头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public

# oracle库文件路径
ORALIB =  -L$(ORACLE_HOME)/lib -L.

# oracle的oci库
ORALIBS = -lclntsh

# 编译参数
CFLAGS = -g -Wall -Wno-write-strings #-Wno-unused-variable

# 找到当前目录下所有 .cpp 文件
CPPS = $(wildcard *.cpp)

# 过滤掉 _ooci.cpp
SRCS = $(filter-out _ooci.cpp, $(CPPS))

# 生成的可执行文件名
TARGET = $(SRCS:.cpp=)

all: $(TARGET)

# 链接时直接用 SRCS + _ooci.o
$(TARGET): $(SRCS) _ooci.o lib_ooci.so
	$(CXX) $(CFLAGS) -o $@ $(SRCS) $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.o

# 只为 _ooci.cpp 生成 .o 文件
_ooci.o: _ooci.cpp _ooci.h
	$(CXX) $(CFLAGS) -c _ooci.cpp $(ORAINCL)

lib_ooci.so:_ooci.cpp _ooci.h
	gcc $(CFLAGS) -fPIC -shared -o lib_ooci.so _ooci.cpp $(ORAINCL)

clean:
	rm -f $(TARGET) _ooci.o lib_ooci.so
