#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unistd.h>

class ThreadPool {
public:
    ThreadPool(int minNum, int maxNum);
    ~ThreadPool();
    void addTask(std::function<void()> task);
    int getBusyNumber();
    int getAliveNumber();
    void shutdown();

private:
    struct Task {
        std::function<void()> function;
    };

    using TaskQueue = std::queue<Task>; //using new_type = existing_type;

    void addThread();
    void worker();
    void manager();

private:
    std::vector<std::thread>* m_threadIDs; // 线程ID列表
    TaskQueue* m_taskQ; // 任务队列
    std::mutex m_lock; // 互斥锁
    std::condition_variable m_notEmpty; // 条件变量，用于线程等待任务
    int m_minNum; // 最小线程数
    int m_maxNum; // 最大线程数
    int m_busyNum; // 忙线程数
    int m_aliveNum; // 活跃线程数
    int m_exitNum; // 需要退出的线程数
    bool m_shutdown; // 是否关闭线程池
};

#endif