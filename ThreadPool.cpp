#include "ThreadPool.h"

ThreadPool::ThreadPool(int minNum, int maxNum) : m_minNum(minNum), m_maxNum(maxNum), m_busyNum(0), m_aliveNum(minNum), m_exitNum(0), m_shutdown(false) {
    m_taskQ = new TaskQueue();
    m_threadIDs = new std::vector<std::thread>;

    for(int i = 0; i < minNum; i++) {
        addThread(); // 添加线程
    }

    std::thread([&] { manager(); }).detach(); // 启动管理者线程
}

ThreadPool::~ThreadPool() {
    shutdown(); // 确保在析构时关闭线程池
}

void ThreadPool::addTask(std::function<void()> task) {
    {
        std::unique_lock<std::shared_mutex> lock(m_taskLock);
        if (m_shutdown) {
            std::cerr << "ThreadPool is shutdown. Cannot add new tasks." << std::endl;
            return;
        }
        Task taskObj{std::move(task)};
        m_taskQ->push(taskObj); // 添加任务到队列
    }
    m_notEmpty.notify_one(); // 通知一个等待的线程有新任务
}

int ThreadPool::getBusyNumber() {
    std::lock_guard<std::mutex> lock(m_countLock);
    return m_busyNum;
}

int ThreadPool::getAliveNumber() {
    std::lock_guard<std::mutex> lock(m_countLock);
    return m_aliveNum;
}

void ThreadPool::addThread() {
    m_threadIDs->emplace_back([&] { worker(); });
    {
        std::lock_guard<std::mutex> lock(m_countLock);
        ++m_aliveNum;
    }
}

void ThreadPool::worker() {
    while(!m_shutdown) {
        Task task;

        {
            std::unique_lock<std::shared_mutex> lock(m_taskLock);
            m_notEmpty.wait(lock, [&] { return !m_taskQ->empty() || m_shutdown; }); // 等待任务队列不为空或线程池关闭

            if (m_shutdown) return;

            task = m_taskQ->front();
            m_taskQ->pop(); // 从队列中移除任务
            {
                std::lock_guard<std::mutex> lock(m_countLock);
                ++m_busyNum; // 忙线程数加1
            }
        }

        task.function(); // 执行任务

        {
            std::lock_guard<std::mutex> lock(m_countLock);
            --m_busyNum; // 忙线程数减1

            // 如果线程池正在减少线程数量且当前线程是多余的，则退出
            if (m_exitNum > 0) {
                --m_aliveNum;
                --m_exitNum;
                return; // 多余的线程退出
            }
        }
    }
}

void ThreadPool::manager() {
    const int scale = 2;
    const int adjustPeriod = 5;

    while (!m_shutdown) {
        std::this_thread::sleep_for(std::chrono::seconds(adjustPeriod));// 每隔一段时间检查一次线程池状态

        if (m_shutdown) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(m_countLock);
            int queueSize;
            {
                std::shared_lock<std::shared_mutex> taskLock(m_taskLock); // 读锁
                queueSize = m_taskQ->size();  // 读取任务队列大小
            }
            int busyNum = m_busyNum;
            int aliveNum = m_aliveNum;

            // 如果任务队列长度大于忙线程数且活跃线程数小于最大线程数，增加线程. 如果忙线程数小于活跃线程数的一半且活跃线程数大于最小线程数，减少线程
            if (queueSize > busyNum && aliveNum < m_maxNum) {
                for (int i = 0; i < scale && m_aliveNum < m_maxNum; ++i) {
                    addThread();
                }
            } else if (busyNum * 2 < aliveNum && aliveNum > m_minNum) {
                m_exitNum = scale;
                for(int i = 0; i < scale && m_aliveNum > m_minNum; ++i) {
                    {
                        std::unique_lock<std::shared_mutex> taskLock(m_taskLock);
                        m_taskQ->push(Task());
                    }
                }
                m_notEmpty.notify_all();
            }
        }
    }
}

void ThreadPool::shutdown() {
    if (m_shutdown) return; // 如果线程池已经关闭，直接返回

    m_shutdown = true; // 设置线程池关闭标志

    m_notEmpty.notify_all(); // 通知所有线程任务队列不为空

    // 等待所有线程结束
    for (auto& thread : *m_threadIDs) {
        thread.join();
    }

    delete m_threadIDs; // 释放线程ID列表内存
    m_threadIDs = nullptr;

    delete m_taskQ; // 释放任务队列内存
    m_taskQ = nullptr;
}

