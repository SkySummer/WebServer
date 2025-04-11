#include "threadpool.h"

ThreadPool::ThreadPool(const size_t thread_count) : stop_(false) {
    // 创建并启动指定数量的线程
    for (size_t i = 0; i < thread_count; i++) {
        workers_.emplace_back([this] { this->workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();

    // 等待所有线程结束
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(queue_mutex_);
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

void ThreadPool::workerLoop() {
    while (!stop_) {
        std::function<void()> task;

        {
            std::unique_lock lock(queue_mutex_);

            // 等待任务队列不为空或线程池停止
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            // 如果已经停止且没有任务，退出线程
            if (stop_ && tasks_.empty()) {
                return;
            }

            // 取出一个任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // 执行任务
        task();
    }
}
