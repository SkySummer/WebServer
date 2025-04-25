#include "core/threadpool.h"

#include <format>

#include "utils/logger.h"

ThreadPool::ThreadPool(const size_t thread_count, Logger* logger) : stop_(false), logger_(logger) {
    // 创建并启动指定数量的线程
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this, i] { this->workerLoop(i); });
    }
    logger_->log(LogLevel::INFO, std::format("Thread pool started with {} threads.", thread_count));
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
        std::lock_guard lock(tasks_mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool has been stopped. Cannot enqueue new tasks.");
        }
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

void ThreadPool::workerLoop(size_t thread_id) {
    while (!stop_) {
        std::function<void()> task;

        {
            std::unique_lock lock(tasks_mutex_);

            // 等待任务队列不为空或线程池停止
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            // 如果已经停止且没有任务，退出线程
            if (stop_ && tasks_.empty()) {
                logger_->log(LogLevel::DEBUG, std::format("Thread {} exiting.", thread_id));
                return;
            }

            // 取出一个任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // 执行任务
        try {
            task();
        } catch (const std::exception& e) {
            logger_->log(LogLevel::ERROR, std::format("Thread {} exception: {}", thread_id, e.what()));
        } catch (...) {
            logger_->log(LogLevel::ERROR, std::format("Thread {} unknown exception.", thread_id));
        }
    }
}
