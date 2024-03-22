#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <queue>
#include <functional>
#include <mutex>
#include <tuple>

class task_queue {
    using task_queue_implementation = std::queue<std::tuple<int, std::function<void(int)>>>;

public:
    task_queue() = default;
    ~task_queue() { clear(); }
    bool empty() const;
    size_t size() const;
    void clear();
    bool pop(std::tuple<int, std::function<void(int)>> &task);
    bool emplace(std::tuple<int, std::function<void(int)>> task);
    

private:
    mutable std::mutex m_mutex;
    task_queue_implementation m_tasks;
};

#include "task_queue.cpp"

#endif // TASK_QUEUE_H
