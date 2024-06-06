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


//分块文件
std::queue<std::string> splits;
// 分块大小，可以根据需要调整
const int SPLIT_SIZE = 1024*64; // 例如，64KB
// 分块编号
int split_num = 1;

//文件分割
void split(){

        buttonrpc client;
        client.as_client("127.0.0.1",8080);

	while(1){
		std::string file = client.call<std::string>("assignTask").val();	
		if (file == "empty"){
                        std::cerr << "map队列为空" << std::endl;
			return;
		}
		std::ifstream ifs(file);
		if(!ifs.is_open()){
			std::cerr << "无法打开输入文件：" << file << std::endl;
			client.call<void>("setMapState",file,false);//重新加入任务队列
		}
		// 缓冲区
		char* buffer = new char[SPLIT_SIZE];
		// 读取并分块文件
		while (!ifs.eof()) {
			ifs.read(buffer, SPLIT_SIZE);
			std::streamsize read_size = ifs.gcount();

			if (read_size > 0) {
			    std::string output_file_name = "split_" + std::to_string(split_num) + ".txt";
			    std::ofstream output_file(output_file_name);
			    
			    if (!output_file.is_open()) {
				std::cerr << "无法创建输出文件" << std::endl;
			    }
			    
			    // 将分块数据写入新文件
			    output_file.write(buffer, read_size);
			    output_file.close();
			    
			    std::cout << "已创建分块文件 : " << output_file_name << std::endl;
			    split_num++;
			    splits.push(output_file_name);
			}
		}

		delete[] buffer;
		ifs.close();	
		client.call<void>("setMapState",file,true);				
	}
}


// map中间文件编号
int mr_num = 1;

bool writeToDisk(std::map<std::string,int>& interMap , string file){
	
	std::string mrfile = "mr_" + file;
	std::ofstream ofs(mrfile);
	//------------------- 测试map失败情况（master重新加入map任务队列）-----------------
	//if(outNum==1){
	//	std::cout << "map inter error" << std::endl;
	//	outNum++;
	//	//std::this_thread::sleep_for(std::chrono::seconds(3));
	//	 // 尝试删除文件
	//	if (std::remove(outfile.c_str()) == 0) {
	//		std::cout << "delete file success!" << std::endl;
	//	} else {
	//		std::cout << "delete file error!" << std::endl;
	//	}
	//	std::cout << "close ofs"<<std::endl;
	//	ofs.close();
	//	return false;
	//}
	//------------------- 测试map失败情况（master重新加入map任务队列）-----------------
	
	if(!ofs.is_open()){
		return false;		
	}
	for(auto& pair : interMap){
		ofs << pair.first << " : " <<pair.second << std::endl; 
        }
	ofs.close();
	mr_num++;
	return true;
}

//锁
std::mutex mtx;
//中间结果
std::map<std::string,int> interMap;

void Map(){
	buttonrpc client;
	client.as_client("127.0.0.1",8080);
	while(1){
		mtx.lock();
                if(splits.size() <= 0){
			std::cerr << "split 文件处理完成" << std::endl;
			return;
		}
                std::string file = splits.front();
		splits.pop();
		mtx.unlock();//解锁

		std::cerr << "获取split文件 : " << file << std::endl;
	        std::ifstream ifs(file);
        	if(!ifs.is_open()){
                	std::cerr << file << "打开失败!" <<std::endl;
			splits.push(file);
			continue;
        	}
		//读取文件内容
		std::string line;
		while(std::getline(ifs,line)){
			std::istringstream iss(line);
			std::string word;
			while(iss >> word){
				interMap[word]++;
			}
		}
	
		//生成中间文件
		if(!writeToDisk(interMap,file)){
			client.call<void>("setMapState",file,false);
			continue;	
		}
		
		//通知master 当前map文件完成
		client.call<void>("setMapState",file,true);
		std::cerr << "文件[" << file << "]处理完成！" << std::endl;
        }
}

void reduce(const std::string& word,const std::vector<int>& counts,std::map<std::string,int>& output){
	
}



int main(){

        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        client.set_timeout(5000);
        int ret = client.call<int>("getMapNum").val();
        int ret1 = client.call<int>("getReduceNum").val();
        std::cout << "mapNum:" << ret << " reduceNum:" << ret1 << std::endl;

	split();

	Map();
	
	//Reduce();
};
