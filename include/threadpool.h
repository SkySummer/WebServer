#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

// 简单的线程池实现：用于将任务分发给固定数量的线程执行
class ThreadPool {
public:
    // 构造函数：创建指定数量的工作线程
    explicit ThreadPool(size_t thread_count);

    // 析构函数：停止所有线程并回收资源
    ~ThreadPool();

    // 提交一个任务给线程池执行
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers_; // 工作线程列表
    std::queue<std::function<void()>> tasks_; // 任务队列

    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;

    // 工作线程主循环函数
    void workerLoop();
};

#endif //THREADPOOL_H
