CXX := g++
INCLUDEDIR := -I../
LIBDIR := -L../
LIBS := -l_public

# 搜集所有cpp文件
SRCS := $(wildcard *.cpp)
# 生成对应的可执行文件列表
BINS := $(SRCS:.cpp=)

all:$(BINS)

%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CXXFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)
#	cp -f $@ ../bin/

# 清理目标
clean:
	rm -f $(BINS)