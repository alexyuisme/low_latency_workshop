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
            counter.fetch_add(1, std::memory_order_relaxed);
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
    }

    // std::cout << "counter = " << counter.load(std::memory_order_relaxed) << std::endl;
}
BENCHMARK(BM_DirectSharing);

void BM_FalseSharing(benchmark::State& state)
{
    // Number of total iterations to run
    const int num_iterations = 1 << 20;

    // Number of threads to spawn
    const int num_threads = 4;

    // Number of elements to process per thread
    const int elements_per_thread = num_iterations / num_threads;

    // atomic integers to increment
    std::array<std::atomic<int>, 4> counters = {0, 0, 0, 0};
    
    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // latch.arrive_and_wait(); // Synchronize thread start
        for (int i = 0; i < elements_per_thread; i++) 
        {
            counters[thread_id].fetch_add(1, std::memory_order_relaxed);
        }

        int counter = counters[thread_id].load(std::memory_order_relaxed);
        counters[thread_id].store(0, std::memory_order_relaxed);
        
        final_sum.fetch_add(counter, std::memory_order_relaxed);
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
    }

    // std::cout << "final_sum = " << final_sum.load(std::memory_order_relaxed) << std::endl;
}
BENCHMARK(BM_FalseSharing);

struct AlignedAtomic {
    alignas(128) std::atomic<int> counter = 0;
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
    std::array<AlignedAtomic, 4> counters = {0, 0, 0, 0};
    
    // Atomic integer to increment
    std::atomic<int> final_sum = 0;

    // Lambda for our work
    auto work = [&](int thread_id)
    {
        // latch.arrive_and_wait(); // Synchronize thread start
        for (int i = 0; i < elements_per_thread; i++) 
        {
            counters[thread_id].counter.fetch_add(1, std::memory_order_relaxed);
        }

        int counter = counters[thread_id].counter.load(std::memory_order_relaxed);
        counters[thread_id].counter.store(0, std::memory_order_relaxed);
        
        final_sum.fetch_add(counter, std::memory_order_relaxed);
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
    }
}
BENCHMARK(BM_NoSharing);

// Main macro for the benchmark
BENCHMARK_MAIN();
