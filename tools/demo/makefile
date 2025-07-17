CXX := g++
INCLUDEDIR := -I../../public/
LIBDIR := -L../../public/
LIBS := -l_public

# 搜集所有cpp文件
TEST_SRCS := $(wildcard demo*.cpp)
# 生成对应的可执行文件列表
TEST_BINS := $(TEST_SRCS:.cpp=)

all:$(TEST_BINS)

%: %.cpp
	$(CXX) $(INCLUDEDIR) $(CXXFLAGS) -o $@ $< $(LIBDIR) $(LDFLAGS) $(LIBS)
	cp -f $@ ../bin/

# 清理目标
clean:
	rm -f $(TEST_BINS)