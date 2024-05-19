#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <sstream>

// Map 函数，将文本分割成单词并计数
void map(const std::string& text, std::map<std::string, int>& output) {
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        ++output[word];
    }
}

// Reduce 函数，将单个单词的计数合并
void reduce(const std::string& word, const std::vector<int>& counts, std::map<std::string, int>& output) {
    int sum = 0;
    for (int count : counts) {
        sum += count;
    }
    output[word] = sum;
}


// MapReduce 框架
void mapreduce(const std::vector<std::string>& texts, std::map<std::string, int>& output) {
    // 中间结果，用于存储每个单词的计数
    std::map<std::string, std::vector<int>> intermediate;

    // 使用互斥锁来保护共享资源
    std::mutex mtx;

    // 执行 Map 阶段
    std::vector<std::thread> map_threads;
    for (const auto& text : texts) {
        map_threads.emplace_back([&intermediate, &mtx, text]() {
            std::map<std::string, int> local_output;
            map(text, local_output);
            std::lock_guard<std::mutex> lock(mtx);
            for (const auto& pair : local_output) {
                intermediate[pair.first].push_back(pair.second);
            }
        });
    }
    for (auto& thread : map_threads) {
        thread.join();
    }
    // 执行 Reduce 阶段
    std::vector<std::thread> reduce_threads;
    for (const auto& pair : intermediate) {
        reduce_threads.emplace_back([&output, &mtx, word=pair.first, counts=pair.second]() {
            std::map<std::string, int> local_output;
            reduce(word, counts, local_output);
            std::lock_guard<std::mutex> lock(mtx);
            output[word] = local_output[word];
        });
    }
    for (auto& thread : reduce_threads) {
        thread.join();
    }
}


int main() {
    std::vector<std::string> texts = {
        "hello world",
        "hello mit",
        "world map",
        "world reduce",
    };

    std::map<std::string, int> result;
    mapreduce(texts, result);

    // 输出结果
    for (const auto& pair : result) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    return 0;
}
