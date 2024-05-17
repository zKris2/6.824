# 定义变量
CXX = g++
CXXFLAGS = -std=c++14 -pthread -Wall

# 目标文件
TARGET = MapReduce
OBJS = $(patsubst *.cpp, obj/%.o,$(wildcard src/*.cpp))

# 编译目标
$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS) -o bin/$@ $^

# 编译源文件
$(OBJS):
	$(CXX) -c $(CXXFLAGS) -o $@$<

# 清理目标
clean:
	rm -rf bin/$(TARGET) obj/$(OBJS)

# 默认目标，包括构建和执行
default: $(TARGET)
	./bin/$(TARGET)

