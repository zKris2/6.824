# mit6824

## 前置环境
```
	ubuntu20.04 、docker 、git
```

## 安装buttonrpc环境
```
	# 安装ZeroMQ依赖
	sudo apt update
	sudo apt install libzmq5
	
	# 拉取buttonrpc
	git clone https://gitcode.com/button-chen/buttonrpc.git
	
	- 使用时包含头文件: "buttonrpc/buttonrpc.hpp"
	- Makefile编译参数: -lzmq -pthread
```
	
