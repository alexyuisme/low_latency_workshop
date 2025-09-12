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

// 分析: 
/*
    -   为何这个SpinLock在高竞争情况下会如此高效:

        1.  减少昂贵的 exchange 操作

            lock_.exchange() 是一个 Read-Modify-Write (RMW) 操作，需要在CPU间进行
            缓存一致性同步（MESI协议），非常昂贵
            
            只有在锁可能释放时（内部load看到false后）才执行 exchange，大幅减少RMW
            操作

        2.  更好的缓存友好性

            -   load() 是纯读操作，比exchange便宜得多
            -   多个线程可以同时读取锁状态而不会产生缓存一致性流量
            -   当锁释放时，所有等待的线程几乎能同时观察到变化

        3.  减少总线争用

            -   频繁的 exchange 操作会导致内存总线和缓存一致性协议饱和
            -   实现二让等待线程"安静地"自旋，只在必要时才参与激烈的锁竞争

    -   内存顺序的正确性

        两种实现的内存顺序使用都是正确的：

        -   exchange(..., std::memory_order_acquire)：获取锁时建立acquire语义，确保临
            界区的操作不会重排到前面

        -   load(std::memory_order_relaxed)：在自旋等待时，只需要原子性，不需要同步语
            义，所以relaxed足够且最轻量
    
*/
struct SpinLock {
    std::atomic<bool> lock_ = {false};

    void lock() 
    {
        // 第一种实现方式
        /*
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
        */

        // 第二种实现方式, 性能几乎与第一种等价
        /*
        while(lock_.exchange(true, std::memory_order_acquire))
        {
            while(lock_.load(std::memory_order_relaxed)); //spin
        }
        */

        // 第三种实现方式
        // 第一步：快速路径，假设锁空闲
        if (!lock_.load(std::memory_order_relaxed) && 
            !lock_.exchange(true, std::memory_order_acquire)) {
            return; // 快速获取成功
        }

        while (true)
        {
            // 先等待锁可能释放
            while (lock_.load(std::memory_order_relaxed)) {
                // 可能加入CPU pause指令或线程yield
                // __builtin_ia32_pause(); // x86 pause指令
                // std::this_thread::yield(); // 让出时间片
            }

            // 再次尝试获取
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                break;
            }
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
    ->Threads(4); // Test with 4 threads

BENCHMARK_REGISTER_F(LockBenchmark, SharedMutexTest)->Name("SharedMutex")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_REGISTER_F(LockBenchmark, SpinLockTest)->Name("SpinLock")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4); // Test with 4 threads

BENCHMARK_REGISTER_F(LockFreeBenchmark, LockFreeTest)->Name("LockFreeTest")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

BENCHMARK_MAIN();
