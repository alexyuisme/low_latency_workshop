#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <iostream>
#include <thread>

// Note
/*
    Performance comparison:

    LockFree > SpinLock > Mutex > Shared_Mutex

    Please keep in mind that in our benchmark, the critical section has only 
    one operation but in real scenarios it has more.
*/

template <typename LockType>
class LockBenchmark : public benchmark::Fixture
{
public:
    void SetUp(const benchmark::State&) override
    {
        counter = 0;
    }

    void TearDown(const benchmark::State& state) override
    {
        // std::thread::id this_id = std::this_thread::get_id();
        // int thread_index = state.thread_index();

        // std::cout << "Thread " << thread_index << " (ID: " << this_id 
        //         << ") ran " << state.iterations() << " iterations" << std::endl;
        
        // std::cout << "=== Benchmark Completion Report ===" << std::endl;
        // std::cout << "Requested max iterations: " << state.max_iterations << std::endl;
        // std::cout << "Actual iterations run: " << state.iterations() << std::endl;
        // std::cout << "Final counter: " << counter << std::endl;

    }

protected:
    // std::atomic<int> counter;
    int counter;
    LockType lock; // 模板化的锁对象
};

class LockFreeBenchmark : public benchmark::Fixture
{
public:
    void SetUp(const benchmark::State&) override
    {
        counter = 0;
    }

    void TearDown(const benchmark::State& state) override
    {
        // std::thread::id this_id = std::this_thread::get_id();
        // int thread_index = state.thread_index();

        // std::cout << "Thread " << thread_index << " (ID: " << this_id 
        //         << ") ran " << state.iterations() << " iterations" << std::endl;
        
        // std::cout << "=== Benchmark Completion Report ===" << std::endl;
        // std::cout << "Requested max iterations: " << state.max_iterations << std::endl;
        // std::cout << "Actual iterations run: " << state.iterations() << std::endl;
        // std::cout << "Final counter: " << counter << std::endl;

    }
protected:
    std::atomic<int> counter;
};

BENCHMARK_DEFINE_F(LockFreeBenchmark, LockFreeTest)(benchmark::State& state) {
    for (auto _ : state) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
    state.SetItemsProcessed(state.iterations());
}

struct SpinLock {
    std::atomic<bool> lock_ = {false};

    void lock() 
    {
        for (;;) 
        {
            if (!lock_.exchange(true, std::memory_order_acquire)) 
            {
                break; // 成功获取锁, 跳出
            }
            while (lock_.load(std::memory_order_relaxed));
            // {
            //     // 可选：减少CPU占用
            //     #ifdef __x86_64__
            //     __builtin_ia32_pause();
            //     #elif defined(__aarch64__)
            //     asm volatile("yield" ::: "memory");
            //     #else
            //     std::this_thread::yield();
            //     #endif
            // }
        }            
    }

    void unlock() 
    { 
        lock_.store(false, std::memory_order_release);
    }

    /*
        The try_lock() should first check if the lock is free before 
        attempting to acquire it. This would prevent excessive coherency 
        traffic in case someone loops over try_lock().
    */
    bool try_lock() noexcept
    {
        // First do a relaxed load to check if lock is free in order 
        // to prevent unnecessary cache misses if someone does while (!try_lock())
        return !lock_.load(std::memory_order_relaxed) && 
               !lock_.exchange(true, std::memory_order_acquire);
    }
};

// 定义模板化的测试
BENCHMARK_TEMPLATE_DEFINE_F(LockBenchmark, MutexTest, std::mutex)(benchmark::State& state) {
    for (auto _ : state) {
        lock.lock();
        // counter.fetch_add(1, std::memory_order_relaxed);
        counter++;
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE_DEFINE_F(LockBenchmark, SharedMutexTest, std::shared_mutex)(benchmark::State& state) {
    for (auto _ : state) {
        lock.lock();
        // counter.fetch_add(1, std::memory_order_relaxed);
        counter++;
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE_DEFINE_F(LockBenchmark, SpinLockTest, SpinLock)(benchmark::State& state) {
    for (auto _ : state) {
        lock.lock();
        // counter.fetch_add(1, std::memory_order_relaxed);
        counter++;
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(LockBenchmark, MutexTest)->Name("Mutex")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_REGISTER_F(LockBenchmark, SharedMutexTest)->Name("SharedMutex")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_REGISTER_F(LockBenchmark, SpinLockTest)->Name("SpinLock")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_REGISTER_F(LockFreeBenchmark, LockFreeTest)->Name("LockFreeTest")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_MAIN();
