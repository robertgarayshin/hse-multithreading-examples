#pragma once

#include <exception>
#include <memory>
#include <mutex>
#include <optional>

template <typename T>
class Future {
public:
    struct State {
        std::mutex mtx;
        std::condition_variable cv;
        std::optional<T> value;
        std::exception_ptr exc;
        bool ready = false;
    };

    explicit Future(std::shared_ptr<State> state) : state_(std::move(state)) {}

    T Get() {
        std::unique_lock lock(state_->mtx);
        state_->cv.wait(lock, [&] { return state_->ready; });
        if (state_->exc) std::rethrow_exception(state_->exc);
        return std::move(*state_->value);
    }

private:
    std::shared_ptr<State> state_;
};

template <>
class Future<void> {
public:
    struct State {
        std::mutex mtx;
        std::condition_variable cv;
        std::exception_ptr exc;
        bool ready = false;
    };

    explicit Future(std::shared_ptr<State> state) : state_(std::move(state)) {}

    void Get() const {
        std::unique_lock lock(state_->mtx);
        state_->cv.wait(lock, [&] { return state_->ready; });
        if (state_->exc) std::rethrow_exception(state_->exc);
    }

private:
    std::shared_ptr<State> state_;
};

template <typename T>
class Promise {
public:
    Promise() : state_(std::make_shared<typename Future<T>::State>()) {}

    Future<T> GetFuture() { return Future<T>(state_); }

    void SetValue(T value) {
        {
            std::lock_guard lock(state_->mtx);
            state_->value = std::move(value);
            state_->ready = true;
        }
        state_->cv.notify_all();
    }

    void SetException(std::exception_ptr exc) {
        {
            std::lock_guard lock(state_->mtx);
            state_->exc = exc;
            state_->ready = true;
        }
        state_->cv.notify_all();
    }

private:
    std::shared_ptr<typename Future<T>::State> state_;
};

template <>
class Promise<void> {
public:
    Promise() : state_(std::make_shared<Future<void>::State>()) {}

    [[nodiscard]] Future<void> GetFuture() const { return Future<void>(state_); }

    void SetValue() const {
        {
            std::lock_guard lock(state_->mtx);
            state_->ready = true;
        }
        state_->cv.notify_all();
    }

    void SetException(const std::exception_ptr& exc) const {
        {
            std::lock_guard lock(state_->mtx);
            state_->exc = exc;
            state_->ready = true;
        }
        state_->cv.notify_all();
    }

private:
    std::shared_ptr<Future<void>::State> state_;
};
