#include "worker.hh"
#include <memory>

namespace lsm_tree {

Worker::Worker() : closed_(false) {}

Worker::~Worker() { delete thread_; }

auto Worker::NewBackgroundWorker() -> std::shared_ptr<Worker> {
  auto worker     = std::make_shared<Worker>();
  worker->thread_ = new std::thread([](Worker *w) { w->Run(); }, worker);
  return worker;
}

void Worker::Join() { thread_->join(); }

void Worker::Stop() {
  std::lock_guard<mutex> lock(work_queue_mutex_);
  closed_ = true;
  work_queue_cond_.notify_all();
}

void Worker::Add(std::function<void()> &&function) noexcept {
  std::lock_guard<mutex> lock(work_queue_mutex_);
  work_queue_.push(std::move(function));
  work_queue_cond_.notify_one();
}

void Worker::Run() { return this->operator()(); }

void Worker::operator()() {
  while (!closed_) {
    std::unique_lock<mutex> lock(work_queue_mutex_);
    while (!closed_ && work_queue_.empty()) {
      work_queue_cond_.wait(lock);
    }
    if (closed_) {
      break;
    }
    auto task = work_queue_.front();
    work_queue_.pop();
    lock.unlock();
    task();
  }
}

}  // namespace lsm_tree