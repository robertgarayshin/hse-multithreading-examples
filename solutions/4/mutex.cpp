#include "mutex.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {

void FutexWait(std::atomic<int>* addr, int expected) {
    syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
}

void FutexWake(std::atomic<int>* addr, int count) {
    syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

}

void Mutex::lock() {
    int c = 0;
    if (m_state.compare_exchange_strong(c, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        return;
    }

    if (c != 2) {
        c = m_state.exchange(2, std::memory_order_acquire);
    }

    while (c != 0) {
        FutexWait(&m_state, 2);
        c = m_state.exchange(2, std::memory_order_acquire);
    }
}

void Mutex::unlock() {
    if (m_state.exchange(0, std::memory_order_release) != 1) {
        FutexWake(&m_state, 1);
    }
}
