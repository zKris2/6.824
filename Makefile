# 定义编译器和编译标志
CC = g++
CFLAGS = -Wall -g -std=c++11

# 定义lzmq和buttonrpc的库路径和链接标志
LIBS = -lzmq -lpthread

# 定义目标可执行文件
TARGET_MASTER = master
TARGET_WORKER = worker
BINDIR = bin

# 定义所有目标和依赖
all: $(BINDIR)/$(TARGET_MASTER) $(BINDIR)/$(TARGET_WORKER)

# 编译master目标
$(BINDIR)/$(TARGET_MASTER): src/master.cpp
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

# 编译worker目标
$(BINDIR)/$(TARGET_WORKER): src/worker.cpp
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

# 清理目标
clean:
	rm -f $(BINDIR)/$(TARGET_MASTER) $(BINDIR)/$(TARGET_WORKER) split-* map-split-* out-put