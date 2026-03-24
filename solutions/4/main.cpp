#include "mutex.h"

#include <cassert>
#include <format>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    static constexpr int ThreadCount = 8;
    static constexpr int IterationsPerThread = 100'000;

    Mutex mutex;
    long long counter = 0;

    std::vector<std::jthread> threads;
    threads.reserve(ThreadCount);

    for (int i = 0; i < ThreadCount; ++i) {
        threads.emplace_back([&] {
            for (int iter = 0; iter < IterationsPerThread; ++iter) {
                mutex.lock();
                ++counter;
                mutex.unlock();
            }
        });
    }

    threads.clear();

    constexpr long long expected = static_cast<long long>(ThreadCount) * IterationsPerThread;
    std::cout << std::format("counter = {} (expected {})\n", counter, expected);

    assert(counter == expected);
    std::cout << "OK\n";
}
