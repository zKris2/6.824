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
#include <queue>
#include <mutex>

#define RPC_IP "127.0.0.1"
#define RPC_PORT 8080

bool Write2Disk(std::map<std::string,int>& map, const string& file){
	std::ofstream ofs(file);
	if(!ofs.is_open()){
		std::cerr << "输出文件[" << file << "]打开失败" << std::endl;
		return false;		
	}
	for(const auto& pair : map){
		ofs << pair.first << " : " <<pair.second << std::endl; 
        }
	ofs.close();
	return true;
}
void Del4Disk(const std::string& file){
		
}

//中间结果
std::map<std::string,std::vector<int>> inter_map;

bool MapHelper(std::string file,std::map<std::string,int>& output){
	std::ifstream ifs(file);
	if(!ifs.is_open()){
		std::cerr << "打开文件-" << file << "-失败" << std::endl;
		return false;
	}
	std::string line;
	while(std::getline(ifs,line)){
		std::istringstream iss(line);
		std::string word;
		while(iss >> word){
			output[word]++;
		}
	}
	ifs.close();
	return true;	
}

void Map(){
	buttonrpc client;
	client.as_client("127.0.0.1",8080);
	
	while(1){
		std::string file = client.call<std::string>("assignMapTask","001").val();
		if (file == "empty"){
                        return;
		}
		std::cerr << "获取文件 : " << file << std::endl;
		std::map<std::string,int> output;
		if(!MapHelper(file,output)){
			std::cerr << file << "Map 失败" << std::endl;	
			client.call<void>("setMapState",file,false);
		}else{
			Write2Disk(output,"map-"+file);
			client.call<void>("setMapState","map-"+file,true);
		}
        }
}

void Shuffle(){
	buttonrpc client;
        client.as_client("127.0.0.1",8080);
	while(1){
		std::string file = client.call<std::string>("assignReduceTask","001").val();
		if(file=="empty"){
			return;
		}
		std::ifstream ifs(file);
		if(!ifs.is_open()){
			std::cerr <<"reduce-"<< file << "-打开失败" << std::endl;
			client.call<void>("setReduceState",file,false);
			continue;
		}
		std::string line;
		while(std::getline(ifs,line)){
			std::istringstream iss(line);
			std::string key;
			int value;
			 // 读取键
			if (getline(iss, key, ':')) {
			    // 跳过空格
			    iss >> ws;
			    // 读取值
			    if (iss >> value) {
				// 将键值对插入map中，如果键已存在，则会更新其值
				inter_map[key].emplace_back(value);
			    }
			}
		}
		ifs.close();
	}
}
void Reduce(){
	std::map<std::string,int> output;
	for (auto it = inter_map.begin();it!=inter_map.end();it++){
		int sum = 0;
		for(int i=0;i<(*it).second.size();i++){
			sum += (*it).second[i];
		}
		output[(*it).first] = sum;
	}
	Write2Disk(output,"out-put");
}

int main(){
        
	buttonrpc client;
        client.as_client("127.0.0.1",8080);
        client.set_timeout(5000);
        int ret = client.call<int>("getMapNum").val();
        int ret1 = client.call<int>("getReduceNum").val();
        std::cout << "mapNum:" << ret << " reduceNum:" << ret1 << std::endl;

	Map();
	Shuffle();
	Reduce();
};
