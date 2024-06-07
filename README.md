# mit6824

## 前置环境
```
	mac 、docker 、git、gcc/g++、make、gdb、buttonrpc、zmq
```
## 开发环境
```
	docker run -itd --name cpp -v /home/cpp:/home/cpp ubuntu:20.04
```
## 安装buttonrpc环境
```
	# 安装ZeroMQ依赖
	sudo apt update
	sudo apt install libzmq3-dev
	
	# 拉取buttonrpc
	git clone https://gitcode.com/button-chen/buttonrpc.git
	
	- 使用时包含头文件: "buttonrpc/buttonrpc.hpp"
	- Makefile编译参数: -lzmq
```
	
## 使用说明
	- 1、src目录下为源文件
	- 2、bin目录下为可执行文件
	- 3、项目使用Makefile进行管理
	- 4、根目录下其他文件为生成文件

## dev-mac分支最新测试执行命令
```
	bin/master src/pg文件

	bin/worker
```

