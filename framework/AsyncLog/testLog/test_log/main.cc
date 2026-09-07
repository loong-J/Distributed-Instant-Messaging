// #include "Logger.h"

// int main() {
//     // 1. 设置级别（可选，默认INFO）
//     Logger::setLogLevel(Logger::DEBUG); 

//     // 2. 各种测试
//     LOG_TRACE << "This is trace message (with func name)"; // 会打印 main()
//     LOG_DEBUG << "This is debug message";
//     LOG_INFO << "Server started";
    
//     // 模拟一个错误
//     int fd = -1;
//     if (fd < 0) {
//         // LogStream 需要支持 << int 等基础类型
//         LOG_ERROR << "Failed to open file, fd=" << fd;
//     }

//     // 3. 致命错误测试（会杀掉进程）
//     // LOG_FATAL << "Memory corruption detected!";
//     LOG_INFO << "Server end!";
//     return 0;
// }











// // #include "Logger.h"
// // #include <unistd.h>
// // #include <iostream>
// // #include "AsyncLogging.h"
// // #include <algorithm>


// // // 全局异步日志对象
// // AsyncLogging* g_asyncLog = nullptr;

// // void asyncOutput(const char* msg, int len) {
// //     if (g_asyncLog) {
// //         g_asyncLog->append(msg, len);
// //     }
// // }



// // void benchmark_test1() {
// //     int cnt = 1000000; // 100万次
// //     auto start = std::chrono::high_resolution_clock::now();
    
// //     for (int i = 0; i < cnt; ++i) {
// //         LOG_INFO << "Benchmark log message aaaaaaaaa" << i;
// //     }

// //     auto end = std::chrono::high_resolution_clock::now();
// //     double cost = std::chrono::duration<double>(end - start).count();
    
// //     printf("总耗时: %.4f秒, 吞吐量: %.0f 条/秒\n", cost, cnt / cost);
// // }

// // void latency_test() {
// //     std::vector<double> latencies;
// //     latencies.reserve(100000);
    
// //     for (int i = 0; i < 100000; ++i) {
// //         auto t1 = std::chrono::high_resolution_clock::now();
// //         LOG_INFO << "test";
// //         auto t2 = std::chrono::high_resolution_clock::now();
// //         latencies.push_back(std::chrono::duration<double>(t2 - t1).count());
// //     }
    
// //     // 排序并计算 P99
// //     std::sort(latencies.begin(), latencies.end());
// //     printf("P99 延迟: %.6f 微秒\n", latencies[99000] * 1e6);
// //     printf("最大延迟: %.6f 微秒\n", latencies.back() * 1e6);
// // }


// // int main(int argc, char* argv[]) {
// //     // 1. 设置最大 Buffer 大小（可以在 AsyncLogging 构造中调整）
// //     // 2. 初始化异步日志，每 3 秒刷新一次
// //     AsyncLogging log(::basename(argv[0]), 3);
    
// //     // 3. 设置全局指针
// //     g_asyncLog = &log;
    
// //     // 4. 劫持 Logger 输出
// //     Logger::setOutput(asyncOutput);
    
// //     // 5. 启动后台线程
// //     log.start();

// //     // // 6. 开始业务逻辑
// //     // LOG_INFO << "Server Start...";
// //     // LOG_INFO << "Testing LogStream: int=" << -123 << " float=" << 3.14;
    
// //     // LOG_INFO << "test Fmt :" << Fmt("%05d", -405) << " over ";

// //     benchmark_test1();// 基准测试
// //     // latency_test();// 记录每次调用 LOG_INFO 的耗时，计算 P99 : 0.903000 微秒   最大延迟: 476.455000 微秒

// //     // 确保程序退出前日志线程有时间处理（生产环境通常是在主循环退出后调用 stop）或者依赖 AsyncLogging 的析构函数
// //     std::this_thread::sleep_for(std::chrono::seconds(5));
// //     return 0;
// // }




#include "AsyncLogging.h"
#include "LogFile.h"
#include "Logger.h"
#include <unistd.h>

// 1. 定义唯一的 AsyncLogging 实例（通常用智能指针或全局指针）
AsyncLogging* g_asyncLog = nullptr;

// 2. 定义桥接函数：Logger 产生日志后，回调这个函数
void asyncOutput(const char* msg, int len) {
    if (g_asyncLog) {
        g_asyncLog->append(msg, len);
    }
}

int main(int argc, char* argv[]) {
    // 3. 初始化后端日志系统

    AsyncLogging log("MyServer", 500*1024*1024); 
    
    // 4. 保存全局指针，供 asyncOutput 使用
    g_asyncLog = &log;
    
    // 5. 设置 Logger 的全局输出回调
    // 这一步之后，项目中任何地方调用 LOG_INFO，都会走进 asyncOutput，进而写入 AsyncLogging
    Logger::setOutput(asyncOutput);
    
    // 6. 启动后端日志线程
    log.start();

    // --- 至此，日志系统初始化完毕 ---

    LOG_INFO << "Server initialized successfully.";

    // 启动你的业务逻辑
    // MyServer server;
    // server.start();
    
    return 0; // 程序退出时，log 对象析构，会自动 flush 剩余日志
}