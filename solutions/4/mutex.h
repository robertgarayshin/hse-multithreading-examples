#pragma once

#include <atomic>

class Mutex {
public:
    Mutex() = default;
    ~Mutex() = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock();
    void unlock();

private:
    std::atomic<int> m_state{0};
};
