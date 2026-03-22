#pragma once

#include <functional>
#include <thread>
#include <vector>

template <typename T>
void ApplyFunction(std::vector<T>& data, const std::function<void(T&)>& transform, const int threadCount = 1) {
    if (data.empty()) return;

    const size_t n = data.size();

    const size_t n_threads = std::min(static_cast<size_t>(threadCount), n);

    std::vector<std::thread> workers;
    workers.reserve(n_threads);

    const size_t chunk = n / n_threads;
    const size_t remainder = n % n_threads;
    size_t start = 0;

    for (size_t i = 0; i < n_threads; ++i) {
        size_t end = start + chunk + (i < remainder ? 1 : 0);
        workers.emplace_back([&data, &transform, start, end]() {
            for (size_t j = start; j < end; ++j) {
                transform(data[j]);
            }
        });
        start = end;
    }

    for (auto& t : workers) {
        t.join();
    }
}
