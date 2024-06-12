#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>
#include "buttonrpc/buttonrpc.hpp"

// 文件分块
#define SPLIT_SIZE 1024 * 1024 // 1M
// 任务完成时间
#define MAP_TASK_TIMEOUT 3
#define REDUCE_TASK_TIMEOUT 5

class Master
{
public:
    bool isMapDone();
    Master(int mapNum = 8, int reduceNum = 8); // 构造函数
    void GetAllFiles(char *file[], int argc);  // 从args[]里获取待处理的文件名
    void Split(const std::string &file);

    int getMapNum() { return m_maps.size(); }
    int getReduceNum() { return m_reduces.size(); }

    std::string assignMapTask(std::string workerID);
    std::string assignReduceTask(std::string workerID);

    void waitTime(std::string type);
    void waitMap(std::string file);
    void waitMapTask(std::string file);
    void waitReduce(std::string file);
    void waitReduceTask(std::string file);

    void setMapState(std::string file);
    void setReduceState(std::string file);

private:
    std::mutex m_assign_mtx;
    bool m_done;
    int m_fileNum; // 命令行文件数

    std::queue<std::string> m_maps; // 所有map任务的工作队列
    int m_mapNum;
    std::vector<std::string> m_runningMaps;              // 正在处理的map任务，分配出去就加入这个队列，用于判断超超时处理重发
    int m_curMapIndex;                                   // 当前正在处理第几个Map任务
    std::unordered_map<std::string, int> m_finishedMaps; // 存放已完成的map任务对应的文件名

    std::queue<std::string> m_reduces; // 所有reduce任务工作队列
    int m_reduceNum;
    std::vector<std::string> m_runningReduces;              // 正在处理的reduce任务，分配出去就加入这个队列，用于判断超超时处理重发
    int m_curReduceIndex;                                   // 当前正在处理第几个Reduce任务
    std::unordered_map<std::string, int> m_finishedReduces; // 已完成的reduce任务对应的编号
};

Master::Master(int mapNum, int reduceNum) : m_done(false), m_mapNum(mapNum), m_reduceNum(reduceNum){};

// 分块编号
int split_num = 1;
// 文件切分
void Master::Split(const std::string &file)
{
    std::ifstream ifs(file);
    if (!ifs.is_open())
    {
        std::cerr << "文件切分出错" << std::endl;
        exit(0);
    }
    char *buffer = new char[SPLIT_SIZE];
    while (!ifs.eof())
    {
        ifs.read(buffer, SPLIT_SIZE);
        std::streamsize read_size = ifs.gcount();

        if (read_size > 0)
        {
            std::string split_file_name = "split-" + std::to_string(split_num);
            std::ofstream ofs(split_file_name);

            if (!ofs.is_open())
            {
                std::cerr << "打开输出文件 : " << split_file_name << " 失败" << std::endl;
                continue;
            }
            // 将分块数据写入新文件
            ofs.write(buffer, read_size);
            ofs.close();

            std::cout << "成功创建分块文件 : " << split_file_name << std::endl;
            // 加入map任务队列
            m_maps.push(split_file_name);
            ++split_num;
        }
    }
    m_fileNum = split_num - 1;
    std::cerr << "待完成文件数量: " << m_fileNum << std::endl;
}

void Master::GetAllFiles(char *file[], int argc)
{
    for (int i = 1; i < argc; i++)
    {
        std::cerr << "argv[" << i << "] : " << file[i] << std::endl;
        // 文件分块
        Split(std::string(file[i]));
    }
    m_fileNum = argc - 1;
};

bool Master::isMapDone()
{
    std::lock_guard<std::mutex> lk(m_assign_mtx);
    if (m_finishedMaps.size() != m_fileNum)
    {
        return false;
    }
    return true;
}

std::string Master::assignMapTask(std::string workerID)
{
    if (isMapDone())
        return "empty";
    if (!m_maps.empty())
    {
        m_assign_mtx.lock();
        std::string file = m_maps.front();
        std::cout << "          分配map文件 [" << file << "] 到 worker[" << workerID << "]" << std::endl;
        m_maps.pop();
        m_assign_mtx.unlock();
        waitMap("map-" + file);
        return file;
    }
    return "empty";
};

void Master::waitTime(std::string type)
{
    if (type == "map")
    {
        std::this_thread::sleep_for(std::chrono::seconds(MAP_TASK_TIMEOUT));
    }
    else if (type == "reduce")
    {
        std::this_thread::sleep_for(std::chrono::seconds(REDUCE_TASK_TIMEOUT));
    }
};

void Master::waitMap(std::string file)
{
    m_assign_mtx.lock();
    m_runningMaps.emplace_back(file); // 加入正在运行的map任务队列
    m_assign_mtx.unlock();
    std::thread t(&Master::waitMapTask, this, file); // 创建线程来检测任务是否完成
    t.detach();
}

void Master::waitMapTask(std::string file)
{
    std::thread t(&Master::waitTime, this, "map");
    t.join(); // 阻塞，等待超时
    m_assign_mtx.lock();
    if (!m_finishedMaps.count(file))
    {
        std::cout << "定时器m - 文件[" << file << "]处理超时!" << std::endl;
        m_maps.push(file);
        std::cout << "定时器m - 重新添加文件[" << file << "]到任务队列" << std::endl;
        m_assign_mtx.unlock();
        return;
    }
    m_assign_mtx.unlock();
    std::cout << "定时器-文件[" << file << "] 处理完成!" << std::endl;
}
void Master::waitReduce(std::string file)
{
    m_assign_mtx.lock();
    m_runningReduces.emplace_back(file); // 加入正在运行的map任务队列
    m_assign_mtx.unlock();
    std::thread t(&Master::waitReduceTask, this, file); // 创建线程来检测任务是否完成
    t.detach();                                         // join方式回收实现超时后解除阻塞
}

void Master::waitReduceTask(std::string file)
{
    std::thread t(&Master::waitTime, this, "reduce");
    t.join(); // 阻塞，等待超时
    m_assign_mtx.lock();
    if (!m_finishedReduces.count(file))
    {
        // 未完成
        std::cout << "定时器r - 文件[" << file << "]处理超时!" << std::endl;
        // 重新加入map任务队列
        m_reduces.push(file);
        std::cout << "定时器r - 重新添加文件[" << file << "]到任务队列" << std::endl;
        m_assign_mtx.unlock();
        return;
    }
    std::cout << "定时器r - 文件[" << file << "] 处理完成!" << std::endl;
    m_assign_mtx.unlock();
}

std::string Master::assignReduceTask(std::string workerID)
{
    if (!m_reduces.empty())
    {
        m_assign_mtx.lock();
        std::string file = m_reduces.front();
        std::cout << "          分配reduce文件 [" << file << "] 到 worker[" << workerID << "]" << std::endl;
        m_reduces.pop();
        m_assign_mtx.unlock();
        waitReduce(file);
        return file;
    }
    return "empty";
};

void Master::setMapState(string file)
{
    std::cerr << "map-file[" << file << "]" << std::endl;
    m_finishedMaps[file] = 1;
    m_reduces.push(file);
}
void Master::setReduceState(string file)
{
    std::cerr << "reduce-file[" << file << "]" << std::endl;
    m_finishedReduces[file] = 1;
}
int main(int argc, char *argv[])
{
    buttonrpc s;
    s.as_server(8080);
    Master master(13, 9);
    master.GetAllFiles(argv, argc);
    s.bind("getMapNum", &Master::getMapNum, &master);
    s.bind("getReduceNum", &Master::getReduceNum, &master);
    s.bind("isMapDone", &Master::isMapDone, &master);
    s.bind("assignMapTask", &Master::assignMapTask, &master);
    s.bind("assignReduceTask", &Master::assignReduceTask, &master);
    s.bind("setMapState", &Master::setMapState, &master);
    s.bind("setReduceState", &Master::setReduceState, &master);

    s.run();
    return 0;
}