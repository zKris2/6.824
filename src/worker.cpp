#include<iostream>
#include "buttonrpc/buttonrpc.hpp"
#include <fstream>
#include <string>
#include <memory>


void map(const char* filepath,std::map<std::string,int>& output){

        buttonrpc client;
        client.as_client("127.0.0.1",8080);
        while(1){
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

        }

        std::fstream file(filepath);
        if(!file.is_open()){
                std::cout<< std::string(filepath) << "open error!" <<std::endl;
        }
        //读取文件内容
        std::string line;
        while(std::getline(file,line)){
                std::istringstream iss(line);
                std::string word;
                while(iss >> word){
                        output[word]++;
                }
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

        std::string task = client.call<std::string>("assignTask").val();
        std::cout << "get task : " << task <<std::endl;
};
