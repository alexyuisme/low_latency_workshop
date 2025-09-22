#include <benchmark/benchmark.h>
#include <array>
#include <atomic>
#include <vector>
#include <thread>
#include <latch>
#include <iostream>

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
BENCHMARK(BM_DirectSharing);

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
BENCHMARK(BM_FalseSharing);

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
BENCHMARK(BM_NoSharing);

// Main macro for the benchmark
BENCHMARK_MAIN();
