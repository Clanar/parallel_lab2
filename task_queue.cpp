#include "task_queue.h"

bool task_queue::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.empty();
}

size_t task_queue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

void task_queue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_tasks.empty()) {
        m_tasks.pop();
    }
}

bool task_queue::pop(std::tuple<int, std::function<void(int)>> & task) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tasks.empty()) {
        return false;
    }
    task = std::move(m_tasks.front());
    m_tasks.pop();
    return true;
}

bool task_queue::emplace(std::tuple<int, std::function<void(int)>> task ) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tasks.size() >= 20) {
        return false;
    }
    m_tasks.emplace(task);
    return true;
}

