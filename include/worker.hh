#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

namespace lsm_tree {

using std::condition_variable;
using std::function;
using std::mutex;
using std::queue;

class Worker {
 public:
  Worker();
  ~Worker();

  Worker(const Worker &)                     = delete;
  auto operator=(const Worker &) -> Worker & = delete;

  inline void Stop();
  inline void Add(std::function<void()> &&function) noexcept;
  inline void Run();
  inline void Join();
  inline void operator()();

  static auto NewBackgroundWorker() -> std::shared_ptr<Worker>;

 private:
  mutex                   work_queue_mutex_;
  bool                    closed_;
  std::thread            *thread_;  // 线程中运行worker->Run()
  queue<function<void()>> work_queue_;
  condition_variable      work_queue_cond_;
};
}  // namespace lsm_tree
