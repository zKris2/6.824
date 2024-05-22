#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include "buttonrpc/buttonrpc.hpp"

#define MAP_TASK_TIMEOUT 3
#define REDUCE_TASK_TIMEOUT 5

class Master{
	public:
		bool isMapDone();
		Master(int mapNum = 8,int reduceNum = 8);//构造函数
		void GetAllFiles(char* file[],int argc);//从args[]里获取待处理的文件名
		
		int getMapNum(){return m_maps.size();}
		int getReduceNum(){return m_reduces.size();}

		std::string assignTask();//分配map任务，RPC
                void waitTime(std::string type);
		void waitMap(char* file);
                void checkMapTask(char* file);

		void setMapState(std::string file,bool state);
	private:
		bool m_done;
		int m_fileNum;//命令行文件数

		std::queue<char *> m_maps;//所有map任务的工作队列
		int m_mapNum;
		std::vector<std::string> m_runningMaps;//正在处理的map任务，分配出去就加入这个队列，用于判断超超时处理重发
		int m_curMapIndex;//当前正在处理第几个Map任务
		std::unordered_map<std::string,int> m_finishedMaps;//存放已完成的map任务对应的文件名

		std::queue<int> m_reduces;//所有reduce任务工作队列
		int m_reduceNum;		
		std::vector<std::string> m_runningReduces;//正在处理的reduce任务，分配出去就加入这个队列，用于判断超超时处理重发
		int m_curReduceIndex;//当前正在处理第几个Reduce任务		
		std::unordered_map<int,int> m_finishedReduces;//已完成的reduce任务对应的编号
};

bool Master::isMapDone(){
       	std::cout<< "fisished task = " << m_finishedMaps.size() <<std::endl; 
	if(m_finishedMaps.size()!=m_fileNum){
                return false;
        }
        return true;
}

Master::Master(int mapNum,int reduceNum):m_done(false),m_mapNum(mapNum),m_reduceNum(reduceNum){
	m_runningMaps.empty();
	m_finishedMaps.clear();
	m_curMapIndex = 0;
	
	m_runningReduces.clear();
	m_finishedReduces.clear();
	m_curReduceIndex = 0;

	if(m_mapNum <= 0 || m_reduceNum <= 0){
		throw std::exception();
	}
};


void Master::GetAllFiles(char* file[],int argc){
	for(int i=1; i<argc; i++){
		m_maps.push(file[i]);
		std::cout << "argv[" << i << "] : " << file[i] <<std::endl; 
	}	
	m_fileNum = argc-1;	
};

void Master::waitTime(std::string type){
        if(type == "map"){
                std::this_thread::sleep_for(std::chrono::seconds(MAP_TASK_TIMEOUT));
        }else if(type == "reduce"){
                std::this_thread::sleep_for(std::chrono::seconds(REDUCE_TASK_TIMEOUT));
        }
};

void Master::waitMap(char* file){
        m_runningMaps.emplace_back(std::string(file));//加入正在运行的map任务队列

        std::thread t(&Master::checkMapTask,this,file); //创建线程来检测任务是否完成
        t.detach();//join方式回收实现超时后解除阻塞
}

void Master::checkMapTask(char* file){
	std::thread t(&Master::waitTime,this,"map");
	t.join();//阻塞，等待超时
	//超时后检查是否已完成
	if(!m_finishedMaps.count(std::string(file))){
		//未完成
		std::cout << "task file[" << file << "] is timeout!" << std::endl;
		//重新加入map任务队列
		m_maps.push(file);
		std::cout << "readd task file[" << file << "]" << std::endl;  
		return;
	}
	std::cout << "task file[" << file <<"] is finished!" << std::endl;
}

std::string Master::assignTask(){
        std::cout << "maps size : " << m_maps.size()<<std::endl;
        //将工作队列任务放到 正在运行的map任务队列
        if(!m_maps.empty()){
                char* file = m_maps.front();//从工作队列取出一个待map的文件名
                std::cout << "  assign task [file] : "<< file << std::endl;
                m_maps.pop();
		// waitMap(file);
		m_runningMaps.emplace_back(std::string(file));
                return std::string(file);
        }
        return "empty";
};


void Master::setMapState(string file,bool state){
	std::cout << "get notify : " << file << " - " << state <<std::endl; 
	if(state){
		m_finishedMaps[file] = 1;
		std::cout << "task file[" << file <<"] is finished!" << std::endl;
	} else {
		//未完成
                //重新加入map任务队列
		char* refile=new char[file.length()+1];
		strcpy(refile,file.c_str());
                m_maps.push(refile);
                std::cout << "readd task file[" << file << "]" << std::endl;
	} 
}

int main(int argc,char* argv[]) {
        buttonrpc s;
        s.as_server(8080);

        Master master(13,9);
        master.GetAllFiles(argv,argc);
        s.bind("getMapNum",&Master::getMapNum,&master);
        s.bind("getReduceNum",&Master::getReduceNum,&master);
        s.bind("assignTask",&Master::assignTask,&master);
	s.bind("setMapState",&Master::setMapState,&master);

        s.run();
        return 0;
}
