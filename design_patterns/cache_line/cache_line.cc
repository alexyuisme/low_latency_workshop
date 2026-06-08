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
// WRONG: Atomic Contention
/*
    BM_DirectSharing (Atomic Contention): All threads concurrently execute 
    fetch_add on the exact same std::atomic<int>. At the CPU assembly level 
    (e.g., x86_64), this emits a LOCK XADD instruction. 
    
    This does not just invalidate cache lines; it triggers a cache lock or bus 
    lock at the hardware level. The 4 threads are forced to strictly serialize 
    (queue up) to modify that single piece of memory.
*/ 
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

    std::latch start_latch(num_threads);

    // Lambda for our work
    auto work = [&]()
    {
        // start_latch.arrive_and_wait(); // Synchronize thread start(macOS doesn't support!)
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
}
BENCHMARK(BM_DirectSharing);

struct Int {
    int value{0};
};

// WRONG: This causes false sharing
// The alignas(std::hardware_destructive_interference_size) applies to the std::array itself (its starting address). The elements 
// inside std::array<Int, N> are packed tightly together.
template <size_t N>
struct BadCounters {
    alignas(std::hardware_destructive_interference_size) std::array<Int, N> counters;
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
    }
    
    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    std::latch start_latch(num_threads);

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // start_latch.arrive_and_wait(); // Synchronize thread start(macOS doesn't support!)
        for (int i = 0; i < elements_per_thread; i++)
        {
            benchmark::DoNotOptimize(counters.counters[thread_id].value += 1);
        }
        
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

        benchmark::ClobberMemory();
    }

}
BENCHMARK(BM_FalseSharing);

struct alignas(std::hardware_destructive_interference_size) PaddedInt {
    int value{0};
};

template <size_t N>
struct GoodCounters
{
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
    }

    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    std::latch start_latch(num_threads);

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // start_latch.arrive_and_wait(); // Synchronize thread start (macOS doesn't support!)
        for (int i = 0; i < elements_per_thread; i++)
        {
            benchmark::DoNotOptimize(counters.counters[thread_id].value += 1);
        }
        
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

        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_NoSharing);

// Main macro for the benchmark
BENCHMARK_MAIN();
