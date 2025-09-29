#include <benchmark/benchmark.h>
#include <cstring>
#include <vector>
#include <random>

// Generate test data
std::vector<int> generate_test_data(size_t n) {
    std::vector<int> data(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000);
    
    for (size_t i = 0; i < n; i++) {
        data[i] = dist(gen);
    }
    return data;
}

void copy_slow(int* dst, const int* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];  // Compiler must reload src[i] each iteration
    }
}

void copy_fast(int* __restrict__ dst, const int* __restrict__ src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];  // Compiler can optimize aggressively
    }
}

// Benchmark for copy_slow
static void BM_copy_slow(benchmark::State& state) {
    const size_t n = state.range(0);
    auto src_data = generate_test_data(n);
    std::vector<int> dst_data(n);
    
    for (auto _ : state) {
        copy_slow(dst_data.data(), src_data.data(), n);
        benchmark::DoNotOptimize(dst_data); // Prevent optimization
        benchmark::ClobberMemory(); // Force memory writes
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * 
                           int64_t(n) * sizeof(int));
    state.SetLabel(std::to_string(n) + " elements");
}

// Benchmark for copy_fast
static void BM_copy_fast(benchmark::State& state) {
    const size_t n = state.range(0);
    auto src_data = generate_test_data(n);
    std::vector<int> dst_data(n);
    
    for (auto _ : state) {
        copy_fast(dst_data.data(), src_data.data(), n);
        benchmark::DoNotOptimize(dst_data); // Prevent optimization
        benchmark::ClobberMemory(); // Force memory writes
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * 
                           int64_t(n) * sizeof(int));
    state.SetLabel(std::to_string(n) + " elements");
}

// Register benchmarks with different array sizes
BENCHMARK(BM_copy_slow)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_copy_fast)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

// Alternative version with ranged inputs for more granular testing
static void BM_copy_slow_range(benchmark::State& state) {
    const size_t n = state.range(0);
    auto src_data = generate_test_data(n);
    std::vector<int> dst_data(n);
    
    for (auto _ : state) {
        copy_slow(dst_data.data(), src_data.data(), n);
        benchmark::DoNotOptimize(dst_data);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * 
                           int64_t(n) * sizeof(int));
}

static void BM_copy_fast_range(benchmark::State& state) {
    const size_t n = state.range(0);
    auto src_data = generate_test_data(n);
    std::vector<int> dst_data(n);
    
    for (auto _ : state) {
        copy_fast(dst_data.data(), src_data.data(), n);
        benchmark::DoNotOptimize(dst_data);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * 
                           int64_t(n) * sizeof(int));
}

// Use RangeMultiplier for more comprehensive testing
// BENCHMARK(BM_copy_slow_range)->RangeMultiplier(10)->Range(1000, 1000000);
// BENCHMARK(BM_copy_fast_range)->RangeMultiplier(10)->Range(1000, 1000000);

BENCHMARK_MAIN();
