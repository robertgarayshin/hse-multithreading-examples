#pragma once

#include <optional>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(const int cap) : cap_(cap) {}

    void Send(const T& val) {
        std::unique_lock lk(mu_);
        cv_.wait(lk, [&] { return count_ < cap_ || is_closed_; });
        if (is_closed_) {
            throw std::runtime_error("send on closed channel");
        }
        buf_.push_back(val);
        ++count_;
        cv_.notify_all();
    }

    std::optional<T> Recv() {
        std::unique_lock lk(mu_);
        cv_.wait(lk, [&] { return count_ > 0 || is_closed_; });
        if (count_ == 0) {
            return std::nullopt;
        }
        T val = std::move(buf_.front());
        buf_.pop_front();
        --count_;
        cv_.notify_all();
        return val;
    }

    void Close() {
        std::unique_lock lk(mu_);
        is_closed_ = true;
        cv_.notify_all();
    }

private:
    std::deque<T> buf_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::size_t cap_;
    std::size_t count_ = 0;
    bool is_closed_ = false;
};

