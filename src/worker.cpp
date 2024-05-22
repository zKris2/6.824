#include<iostream>
#include "buttonrpc/buttonrpc.hpp"
#include <fstream>
#include <string>
#include <memory>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// 中间文件
int outNum = 1;



bool writeToDisk(std::map<std::string,int>& interMap , string file){
	
	std::string outfile = "inter-map-"+ std::to_string(outNum);
	std::cout<< "outf : " <<outfile<<std::endl;
	std::ofstream ofs(outfile);
	//------------------- 测试map失败情况（master重新加入map任务队列）-----------------
	if(outNum==1){
		std::cout << "map inter error" << std::endl;
		outNum++;
		//std::this_thread::sleep_for(std::chrono::seconds(3));
		 // 尝试删除文件
		if (std::remove(outfile.c_str()) == 0) {
			std::cout << "delete file success!" << std::endl;
		} else {
			std::cout << "delete file error!" << std::endl;
		}
		std::cout << "close ofs"<<std::endl;
		ofs.close();
		return false;
	}
	//------------------- 测试map失败情况（master重新加入map任务队列）-----------------
	
	if(!ofs.is_open()){
		return false;		
	}
	for(auto& pair : interMap){
		ofs << pair.first << " : " <<pair.second << std::endl; 
        }
	ofs.close();
	outNum++;
	return true;
}

void Map(){
        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        
	std::map<std::string,int> interMap;
	while(1){
		interMap.clear();
                bool ret = client.call<bool>("isMapDone").val();
                if(ret){
                        std::cout << "No Task,finished!" << std::endl;
                        return;
                }
                std::string task = client.call<std::string>("assignTask").val();
                if(task == "empty"){
                        continue;
                }
                std::cout << "get task : " << task <<std::endl;
	        std::fstream file(task);
        	if(!file.is_open()){
                	std::cout<< std::string(task) << "open error!" <<std::endl;
        	}
		//读取文件内容
		std::string line;
		while(std::getline(file,line)){
			std::istringstream iss(line);
			std::string word;
			while(iss >> word){
				interMap[word]++;
			}
		}
	
		//生成中间文件
		if(!writeToDisk(interMap,task)){
			client.call<void>("setMapState",task,false);
			continue;	
		}
		
		//通知master
		client.call<void>("setMapState",task,true);
		std::cout << "map is finished!" << std::endl;
        }
}

void reduce(const std::string& word,const std::vector<int>& counts,std::map<std::string,int>& output){
        int sum = 0;
        for(int count : counts){
                sum+=count;
        }
        output[word] = sum;
}


int main(){

        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        client.set_timeout(5000);
        int ret = client.call<int>("getMapNum").val();
        int ret1 = client.call<int>("getReduceNum").val();
        std::cout << "mapNum:" << ret << " reduceNum:" << ret1 << std::endl;

	Map();
	//for(int i = 0 ; i < ret; i++){
	//	std::thread t(Map);
	//	t.detach();
	//}
	//std::this_thread::sleep_for(std::chrono::seconds(5));
};
