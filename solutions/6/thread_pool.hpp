#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include "future.hpp"

class ThreadPool {
public:
    explicit ThreadPool(const size_t num_threads) {
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
            workers_.emplace_back([this] { WorkerLoop(); });
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> Future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        Promise<ReturnType> promise;
        auto future = promise.GetFuture();
        {
            auto task = [p = std::move(promise),
                        bound = [func = std::forward<F>(f),
                            tup  = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                            return std::apply(std::move(func), std::move(tup));
                        }]() mutable {
                try {
                    if constexpr (std::is_void_v<ReturnType>) {
                        bound();
                        p.SetValue();
                    } else {
                        p.SetValue(bound());
                    }
                } catch (...) {
                    p.SetException(std::current_exception());
                }
            };
            std::lock_guard lock(mutex_);
            if (stop_) throw std::runtime_error("ThreadPool: pool is stopped");
            tasks_.push(std::move(task));
        }
        cv_.notify_one();

        return future;
    }

private:
    void WorkerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};
