#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

class Master{
	public:
		Master(int mapNum = 8,int reduceNum = 8);//构造函数
		void GetAllFiles(char* file[],int argc);//从args[]里获取待处理的文件名
		
		int getMapNum(){return m_mapNum;}
		int getReduceNum(){return m_reduceNum;}

	private:
		bool m_done;
		int m_fileNum;//命令行文件数

		std::list<char *> m_maps;//所有map任务的工作队列
		int m_mapNum;
		std::vector<std::string> m_runningMap_task;//正在处理的map任务，分配出去就加入这个队列，用于判断超超时处理重发
		int m_curMapIndex;//当前正在处理第几个Map任务
		std::unordered_map<std::string,int> m_finishedMap_task;//存放已完成的map任务对应的文件名

		std::vector<int> m_reduces;//所有reduce任务工作队列
		int m_reduceNum;		
		std::vector<std::string> m_runningReduce_task;//正在处理的reduce任务，分配出去就加入这个队列，用于判断超超时处理重发
		int m_curReduceIndex;//当前正在处理第几个Reduce任务		
		std::unordered_map<int,int> m_finishedReduce_task;//已完成的reduce任务对应的编号
};

Master::Master(int mapNum,int reduceNum):m_done(false),m_mapNum(mapNum),m_reduceNum(reduceNum){
	m_maps.clear();
	m_runningMap_task.empty();
	m_finishedMap_task.clear();
	m_curMapIndex = 0;
	
	m_reduces.clear();
	m_runningReduce_task.clear();
	m_finishedReduce_task.clear();
	m_curReduceIndex = 0;

	if(m_mapNum <= 0 || m_reduceNum <= 0){
		throw std::exception();
	}
	for(int i=0; i<reduceNum; i++){
		m_reduces.emplace_back(i);
	}
};


void Master::GetAllFiles(char* file[],int argc){
	for(int i=1; i<argc; i++){
		m_maps.emplace_back(file[i]);
		std::cout << "argv[" << i << "] : " << file[i] <<std::endl; 
	}	
	m_fileNum = argc-1;	
	std::cout << "commind num : " << m_fileNum << std::endl; 
};


