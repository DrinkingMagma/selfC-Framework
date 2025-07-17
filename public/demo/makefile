CXX := g++
INCLUDEDIR := -I../
LIBDIR := -L../
LIBS := -l_public -l_ftp

# 搜集所有以"demo_"开头的cpp文件
DEMO_SRCS := $(wildcard demo_*.cpp)
# 生成对应的可执行文件列表
DEMO_BINS := $(DEMO_SRCS:.cpp=)

all:$(DEMO_BINS)

# 测试程序的通用规则：每个 demo_bin 对应一个 demo_src
# 用模式规则实现 "demo_%: demo_%.cpp"
%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CXXFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)

# 清理目标
clean:
	rm -f $(DEMO_BINS)