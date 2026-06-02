#include <benchmark/benchmark.h>
#include <array>
#include <atomic>
#include <vector>
#include <thread>
#include <latch>
#include <iostream>
#include <numeric>
#include <random>

// ==================== Benchmark for Direct Sharing ====================
void BM_DirectSharing(benchmark::State& state)
{
    // Number of total iterations to run
    const int num_iterations = 1 << 20;

    // Number of threads to spawn
    const int num_threads = 4;

    // Number of elements to process per thread
    const int elements_per_thread = num_iterations / num_threads;

    // Atomic integer to increment
    std::atomic<int> counter = 0;

    // Lambda for our work
    auto work = [&]()
    {
        // latch.arrive_and_wait(); // Synchronize thread start
        for (int i = 0; i < elements_per_thread; i++) 
        {
            benchmark::DoNotOptimize(counter.fetch_add(1, std::memory_order_relaxed));
            benchmark::ClobberMemory();
        }
    };

    for(auto _ : state)
    {
        counter.store(0, std::memory_order_relaxed);
        // Spawn threads
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(work);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        benchmark::ClobberMemory();
    }

    // std::cout << "counter = " << counter.load(std::memory_order_relaxed) << std::endl;
}
//BENCHMARK(BM_DirectSharing);

struct Int {
    int value{0};
};

// WRONG: This causes false sharing
template <size_t N>
struct BadCounters {
    // alignas(64) int counters[N];
    alignas(64) std::array<Int, N> counters;
};

void BM_FalseSharing(benchmark::State& state)
{
    // Number of total iterations to run
    const int num_iterations = 1 << 20;

    // Number of threads to spawn
    const int num_threads = 4;

    // Number of elements to process per thread
    const int elements_per_thread = num_iterations / num_threads;

    // atomic integers to increment
    BadCounters<num_threads> counters;
    for (int i = 0; i < num_threads; i++)
    {
        counters.counters[i].value = 0;
        // uintptr_t address = reinterpret_cast<uintptr_t>(&bad_counters.counters[i]);
        // std::cout << "counters[" << i << "] address: " << address
        //           << " (aligned to 64 bytes: " << ((address % 64) == 0 ? "YES" : "NO") << ")\n";
    }
    
    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // latch.arrive_and_wait(); // Synchronize thread start
        for (int i = 0; i < elements_per_thread; i++)
        {
            benchmark::DoNotOptimize(counters.counters[thread_id].value += 1);
        }

        // std::cout << "count = " << bad_counters.counters[thread_id] << std::endl;
        
        benchmark::DoNotOptimize(final_sum.fetch_add(counters.counters[thread_id].value, std::memory_order_relaxed));
        benchmark::DoNotOptimize(counters.counters[thread_id].value = 0);

        benchmark::ClobberMemory();
    };

    for(auto _ : state)
    {
        final_sum.store(0, std::memory_order_relaxed);
        // Spawn threads
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(work, i);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // std::cout << "final_sum = " << final_sum.load(std::memory_order_relaxed) << std::endl;
        benchmark::ClobberMemory();
    }

}
//BENCHMARK(BM_FalseSharing);

struct alignas(64) PaddedInt {
    int value{0};
};

template <size_t N>
struct GoodCounters
{
    // PaddedInt counters[N];
    std::array<PaddedInt, N> counters;
};

void BM_NoSharing(benchmark::State& state)
{
    // Number of total iterations to run
    const int num_iterations = 1 << 20;

    // Number of threads to spawn
    const int num_threads = 4;

    // Number of elements to process per thread
    const int elements_per_thread = num_iterations / num_threads;

    // atomic integers to increment
    GoodCounters<num_threads> counters;
    for (int i = 0; i < num_threads; i++)
    {
        counters.counters[i].value = 0;
        // uintptr_t address = reinterpret_cast<uintptr_t>(&good_counters.counters[i]);
        // std::cout << "counters[" << i << "] address: " << address
        //           << " (aligned to 64 bytes: " << ((address % 64) == 0 ? "YES" : "NO") << ")\n";
    }

    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // latch.arrive_and_wait(); // Synchronize thread start
        for (int i = 0; i < elements_per_thread; i++)
        {
            benchmark::DoNotOptimize(counters.counters[thread_id].value += 1);
        }

        // std::cout << "count = " << good_counters.counters[thread_id].value << std::endl;
        
        benchmark::DoNotOptimize(final_sum.fetch_add(counters.counters[thread_id].value, std::memory_order_relaxed));
        benchmark::DoNotOptimize(counters.counters[thread_id].value = 0);

        benchmark::ClobberMemory();
    };

    for(auto _ : state)
    {
        final_sum.store(0, std::memory_order_relaxed);
        // Spawn threads
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(work, i);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // std::cout << "final_sum = " << final_sum.load(std::memory_order_relaxed) << std::endl;
        benchmark::ClobberMemory();
    }
}
//BENCHMARK(BM_NoSharing);

/*
enum class Age : int {}
Age average_age(const std::vector<Age> ages) {
    int total = 0;
    for (Age age : ages) {
        total += static_cast<int>(age);
    }

    return static_cast<Age>(total / ages.size());
}
*/

template <typename T>
struct AgeWrapper {
    enum class Age : T {};
};

// 為了讓程式碼更乾淨，定義一個型態別名 (Type Alias)
template <typename T>
using Age = typename AgeWrapper<T>::Age;

// The targeted average_age function template
template <typename T>
Age<T> average_age(const std::vector<Age<T>>& ages) {
    int total = 0;
    for (Age<T> age : ages) {
        total += static_cast<int>(age); // Triggers integer promotion for uint8_t and short
    }

    // Guard against empty vectors
    if (ages.empty()) return static_cast<Age<T>>(0);
    
    return static_cast<Age<T>>(total / ages.size());
}

// Helper function to create randomized mock data
template <typename T>
std::vector<Age<T>> generate_mock_data(size_t size) {
    std::vector<Age<T>> data(size);
    std::mt19937 prng(42); // Fixed seed for reproducible benchmarks
    std::uniform_int_distribution<int> dist(1, 100); // Ages 1 to 100

    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<Age<T>>(dist(prng));
    }
    return data;
}

// Generic benchmark wrapper
template <typename T>
static void BM_AverageAge(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto data = generate_mock_data<T>(size);

    for (auto _ : state) {
        auto result = average_age<T>(data);
        benchmark::DoNotOptimize(result);
    }

    // Tracks total items processed per second for throughput evaluation
    state.SetItemsProcessed(state.iterations() * size);
}

/*
// Register benchmarks across diverse vector sizes
// 100: Fits entirely in L1 Cache
// 10,000: Fits in L2/L3 Cache
// 1,000,000: Exceeds standard CPU caches, forcing Main RAM trips
#define REGISTER_BENCHMARK(type_name, type) \
    BENCHMARK_TEMPLATE(BM_AverageAge, type)->Name("BM_Age_" #type_name)->RangeMultiplier(10)->Range(100, 1000000);

REGISTER_BENCHMARK(Int, int);
REGISTER_BENCHMARK(Uint8, uint8_t);
REGISTER_BENCHMARK(Short, short);
*/

// 16KN 代表 16 * 1024 個元素
constexpr int KN = 1024;

// 註冊基準測試並指定 16KN 到 128KN 的線性級距（每次增加 16KN）
// DenseRange(start, end, step) 
BENCHMARK_TEMPLATE(BM_AverageAge, int)     ->Name("BM_Age_Int")  ->DenseRange(16 * KN, 128 * KN, 16 * KN);
BENCHMARK_TEMPLATE(BM_AverageAge, short)   ->Name("BM_Age_Short")->DenseRange(16 * KN, 128 * KN, 16 * KN);
BENCHMARK_TEMPLATE(BM_AverageAge, uint8_t) ->Name("BM_Age_Uint8")->DenseRange(16 * KN, 128 * KN, 16 * KN);

// Main macro for the benchmark
BENCHMARK_MAIN();
