#ifndef CORE_THREADPOOL_H
#define CORE_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

class Logger;

// 简单的线程池实现：用于将任务分发给固定数量的线程执行
class ThreadPool {
public:
    // 构造函数：创建指定数量的工作线程
    explicit ThreadPool(size_t thread_count, Logger* logger);

    // 析构函数：停止所有线程并回收资源
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // 提交一个任务给线程池执行
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers_;  // 工作线程列表
    std::atomic<bool> stop_;

    std::queue<std::function<void()>> tasks_;  // 任务队列
    std::mutex tasks_mutex_;
    std::condition_variable condition_;

    Logger* logger_;  // 日志

    // 工作线程主循环函数
    void workerLoop(size_t thread_id);
};

#endif  // CORE_THREADPOOL_H
