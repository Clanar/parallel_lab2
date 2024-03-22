#include "task_queue.h"
#include <any>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <tuple>

template <typename Result> class thread_pool {
public:
  inline thread_pool() = default;
  inline ~thread_pool() { terminate(); }

public:
  void initialize(const size_t worker_count);
  void terminate();
  void routine();
  bool working() const;
  bool working_unsafe() const;

public:
  template <typename... arguments>
  int add_task(Result (*func)(arguments...), arguments... parameters);

  Result get_results(int id);

private:
  mutable std::shared_mutex m_rw_lock;
  std::condition_variable_any m_task_waiter;
  std::vector<std::thread> m_workers;
  task_queue m_tasks;

  std::map<int, Result> m_results;

  bool m_initialized = false;
  bool m_terminated = false;

  int lastId = 0;
};

template <typename Result>
void thread_pool<Result>::initialize(const size_t worker_count) {
  terminate(); // Ensure previous state is cleaned up
  m_initialized = true;
  m_terminated = false;
  m_workers.reserve(worker_count);
  for (size_t i = 0; i < worker_count; ++i) {
    m_workers.emplace_back([this] { routine(); });
  }
}

template <typename Result> void thread_pool<Result>::terminate() {
  if (!m_initialized)
    return;

  {
    std::unique_lock<std::shared_mutex> lock(m_rw_lock);
    m_terminated = true;
  }
  m_task_waiter.notify_all();

  for (auto &worker : m_workers) {
    if (worker.joinable())
      worker.join();
  }

  m_initialized = false;
}

template <typename Result> void thread_pool<Result>::routine() {
  while (true) {
    std::tuple<int, std::function<void(int)>> task;
    {
      std::unique_lock<std::shared_mutex> lock(m_rw_lock);
      m_task_waiter.wait(lock,
                         [this] { return m_terminated || !m_tasks.empty(); });
      if (m_terminated && m_tasks.empty())
        return; // Thread exits when terminated and no tasks
      m_tasks.pop(task);
    }
    std::get<1>(task)(std::get<0>(task));
  }
}

template <typename Result> bool thread_pool<Result>::working() const {
  std::shared_lock<std::shared_mutex> lock(m_rw_lock);
  return !m_terminated;
}

template <typename Result> bool thread_pool<Result>::working_unsafe() const {
  return !m_terminated;
}

template <typename Result>
template <typename... arguments>
int thread_pool<Result>::add_task(Result (*func)(arguments...),
                                  arguments... parameters) {
  lastId++;

  auto result = std::make_shared<Result>();

  std::function<void(int)> wrapped_task = [=](int id) {
    *result = func(parameters...);
    {
      std::lock_guard<std::shared_mutex> lock(m_rw_lock);
      std::cout << "Adding results to the map " << id << " = " << *result
                << std::endl;
      m_results.emplace(id, std::move(*result));
    }
  };

  {
    std::lock_guard<std::shared_mutex> lock(m_rw_lock);
    m_tasks.emplace({lastId, std::move(wrapped_task)});
    m_task_waiter.notify_one();
  }

  return lastId;
}

template <typename Result> Result thread_pool<Result>::get_results(int id) {
  std::cout << "Getting results from the map " << id << std::endl;
  std::lock_guard<std::shared_mutex> lock(m_rw_lock);
  return m_results[id];
}

int task(int a, int b) { return a + b; }

int main() {
  thread_pool<int> pool;
  pool.initialize(4); // Initialize with 4 worker threads

  int taskId1 = pool.add_task(task, 3, 4);
  int taskId2 = pool.add_task(task, 5, 6);
  int taskId3 = pool.add_task(task, 7, 8);

  // Wait for the tasks to complete
  std::this_thread::sleep_for(std::chrono::seconds(1));

  pool.terminate();
  // Collect the results
  auto results = pool.get_results(taskId2);
  std::cout << "Task 2 result: " << results << std::endl;

  return 0;
}
