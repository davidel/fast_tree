#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace fast_tree {
namespace detail {

template <typename T>
class queue {
 public:
  void stop() {
    std::lock_guard lg(lock_);

    stopped_ = true;
    cv_.notify_all();
  }

  void push(T elem) {
    {
      std::lock_guard lg(lock_);

      queue_.push_back(std::move(elem));
    }
    cv_.notify_one();
  }

  std::optional<T> pop() {
    std::unique_lock ul(lock_);

    cv_.wait(ul, [this]{ return !queue_.empty() || stopped_; });

    std::optional<T> elem;

    if (!queue_.empty()) {
      elem = std::move(queue_.front());
      queue_.pop_front();
    }

    return elem;
  }

 private:
  std::mutex lock_;
  std::condition_variable cv_;
  std::deque<T> queue_;
  bool stopped_ = false;
};

template <typename T>
class result {
 public:
  explicit result(T value) :
      value_(std::move(value)) {
  }

  explicit result(std::exception_ptr exptr) :
      exptr_(std::move(exptr)) {
  }

  T get() {
    if (exptr_ != nullptr) {
      std::rethrow_exception(eptr_);
    }

    return std::move(*value_);
  }

 private:
  std::optional<T> value_;
  std::exception_ptr exptr_;
};

template <typename T>
result<T> run(const std::function<T ()>& fn) {
  try {
    return result<T>(fn());
  } catch (...) {
    return result<T>(std::current_exception());
  }
}

}

class threadpool {
 public:
  using thread_function = std::function<void ()>;

  explicit threadpool(size_t num_threads) {
    threads_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
      threads_.push_back(std::make_unique<std::thread>(
          [this]() {
            run();
          }));
    }
  }

  void stop() {
    function_queue_.stop();
  }

  void push_work(thread_function thread_fn) {
    function_queue_.push(std::move(thread_fn));
  }

 private:
  void run() {
    for (;;) {
      std::optional<thread_function> thread_fn(function_queue_.pop());

      if (!thread_fn) {
        break;
      }
      (*thread_fn)();
    }
  }

  std::vector<std::unique_ptr<std::thread>> threads_;
  detail::queue<thread_function> function_queue_;
};

template <typename T, typename C, typename I>
std::vector<T> map(const std::function<T (const C&)>& fn, I first, I last,
                   size_t num_threads) {

}

}
