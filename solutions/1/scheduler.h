#pragma once

#include <coroutine>
#include <deque>
#include <vector>

class Scheduler;

struct Yield {
  Scheduler &sched;

  static bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) const noexcept;
  static void await_resume() noexcept {}
};

struct Task {
  struct promise_type {
    Task get_return_object() noexcept {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    static std::suspend_always initial_suspend() noexcept { return {}; }
    static std::suspend_always final_suspend() noexcept { return {}; }

    static void return_void() noexcept {}

    static void unhandled_exception() noexcept { std::terminate(); }
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit Task(const Handle h) : handle(h) {}

  Task(Task &&other) noexcept : handle(std::exchange(other.handle, {})) {}

  Task &operator=(Task &&) = delete;

  ~Task() {
    if (handle) {
      handle.destroy();
    }
  }

  Handle handle;
};

class Scheduler {
public:
  void Spawn(Task task) {
    const auto h = task.handle;
    owned_.push_back(std::move(task));
    ready_.push_back(h);
  }

  void Enqueue(const std::coroutine_handle<> h) { ready_.push_front(h); }

  void Run() {
    while (!ready_.empty()) {
      auto h = ready_.back();
      ready_.pop_back();
      if (!h.done()) {
        h.resume();
      }
    }
  }

private:
  std::vector<Task> owned_;
  std::deque<std::coroutine_handle<>> ready_;
};

inline void Yield::await_suspend(const std::coroutine_handle<> h) const noexcept {
  sched.Enqueue(h);
}
