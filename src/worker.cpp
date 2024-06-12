#include <iostream>
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
                ofs << pair.first << " " <<pair.second << std::endl;
        }
        ofs.close();
        return true;
}
void Del4Disk(const std::string& file){

}

bool isAlphaNumeric(const std::string& word) {
    return std::all_of(word.begin(), word.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_'; // 允许下划线作为单词的一部分
    });
}
bool isOnlyLetters(const std::string& word) {
    return std::all_of(word.begin(), word.end(), [](unsigned char c) {
        return std::isalpha(c);
    });
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
                        if(isOnlyLetters(word))
                        {
                                output[word]++;
                        }
                }
        }
        ifs.close();
        return true;
}

std::mutex g_mtx;
void Map(std::string id){
        buttonrpc client;
        client.as_client("127.0.0.1",8080);

        while(1){
                g_mtx.lock();
                if(client.call<bool>("isMapDone").val()){
                        g_mtx.unlock();
                        std::cerr << "map任务已经全部完成" <<  std::endl;
                        return;
                }
                g_mtx.unlock();
                std::string file = client.call<std::string>("assignMapTask",id).val();
                if (file == "empty"){
                        continue;
                }
                std::cerr << "worker[" << id << "] 获取文件 : " << file <<  std::endl;
                std::map<std::string,int> output;
                if(!MapHelper(file,output)){
                        std::cerr << file << "Map 失败" << std::endl;
                }else{
                        Write2Disk(output,"map-"+file);
                        client.call<void>("setMapState","map-"+file);
                        std::cerr << file << "处理完成，通知了master" <<std::endl;
                }
        }
}

void Shuffle(std::string id){
        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        while(1){
                std::string file = client.call<std::string>("assignReduceTask",id).val();
                if(file=="empty"){
                        return;
                }
                std::cerr << "worker[" << id << "] 获取reduce文件 : " << file << std::endl;
                std::ifstream ifs(file);
                if(!ifs.is_open()){
                        std::cerr <<"reduce-"<< file << "-打开失败" << std::endl;
                        continue;
                }
                std::string line,key;
                int value;
                while(std::getline(ifs,line)){
                        // 创建一个字符串流来分割键和值
                        std::stringstream ss(line);
                        // 读取键
                        ss >> key;
                        // 读取值
                        ss >> value;
                        // 将键值对插入到map中
                        inter_map[key].emplace_back(value);
                }
                ifs.close();
                client.call<void>("setReduceState",file);
        }
}
void Reduce(){
        std::map<std::string,int> output;
        for (auto it = inter_map.begin();it!=inter_map.end();it++){
                int sum = 0;
                for(const auto& n : (*it).second){
                        sum += n;
                }
                output[(*it).first] = sum;
        }
        Write2Disk(output,"out-put");
}

void worker(int id){
        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        client.set_timeout(5000);
        int ret = client.call<int>("getMapNum").val();
        int ret1 = client.call<int>("getReduceNum").val();
        std::cout << "mapNum:" << ret << " reduceNum:" << ret1 << std::endl;

        Map(std::to_string(id));
        std::cerr << "线程["<<id << "] 开始shuffle" <<std::endl;
        Shuffle(std::to_string(id));
        Reduce();
}

int main(){
        std::vector<std::thread> threads;
        for (int i=0;i<4;i++){
                threads.emplace_back(worker,i);
        }
        for(auto& i : threads){
                i.join();
        }
};