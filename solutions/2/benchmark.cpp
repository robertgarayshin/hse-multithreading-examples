#include <benchmark/benchmark.h>
#include <cmath>
#include <numeric>
#include "apply_function.h"

static void BM_SmallVectorSingleThread(benchmark::State& state) {
    for (auto _ : state) {
        std::vector data(64, 1);
        ApplyFunction<int>(data, [](int& x) { x += 1; }, 1);
        benchmark::DoNotOptimize(data);
    }
}
BENCHMARK(BM_SmallVectorSingleThread);

static void BM_SmallVectorMultiThread(benchmark::State& state) {
    const int threads = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector data(64, 1);
        ApplyFunction<int>(data, [](int& x) { x += 1; }, threads);
        benchmark::DoNotOptimize(data);
    }
}
BENCHMARK(BM_SmallVectorMultiThread)->Arg(4)->Arg(8);

static auto heavyTransform = [](double& x) {
    for (int i = 0; i < 500; ++i) {
        x = std::sqrt(x + 1.0);
    }
};

static void BM_LargeVectorSingleThread(benchmark::State& state) {
    for (auto _ : state) {
        std::vector data(4096, 2.0);
        ApplyFunction<double>(data, heavyTransform, 1);
        benchmark::DoNotOptimize(data);
    }
}
BENCHMARK(BM_LargeVectorSingleThread);

static void BM_LargeVectorMultiThread(benchmark::State& state) {
    const int threads = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector data(4096, 2.0);
        ApplyFunction<double>(data, heavyTransform, threads);
        benchmark::DoNotOptimize(data);
    }
}
BENCHMARK(BM_LargeVectorMultiThread)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();
