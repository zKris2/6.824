# 定义变量
CXX = g++
CXXFLAGS = -std=c++14 -pthread

# 目标文件
TARGET = MapReduce
OBJS = $(patsubst *.cpp, %.o,$(wildcard src/*.cpp))
# 测试文件
TEST = $(patsubst *.cpp, %.o,$(wildcard test/*.cpp))

# 编译目标
$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS) -o bin/$@ $^

# 编译源文件
$(OBJS):
	$(CXX) -c $(CXXFLAGS) -o $@$<
	

# 清理目标
clean:
	rm -rf bin/$(TARGET) $(OBJS)

# 默认目标，包括构建和执行
default:
	bin/$(TARGET)
