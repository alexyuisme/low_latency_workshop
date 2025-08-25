#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <iostream>
#include <thread>

// 测试 1: benchmark::Fixture
/*
    测试夹具（benchmark::Fixture），它专门为向量相关的基准测试
    提供共享的设置和资源。
*/

// 基础 Fixture 类
class VectorFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        // 在每个测试用例运行前执行
        vec.resize(state.range(0));
        std::iota(vec.begin(), vec.end(), 0);
    }

    void TearDown(const benchmark::State& state) override {
        // 在每个测试用例运行后执行
        vec.clear();
    }

protected:
    std::vector<int> vec;
};

// 使用 BENCHMARK_DEFINE_F 定义基准测试
// VectorFixture 是一个测试夹具（Test Fixture）类，它专门为向量相关的基准测试提供共享的设置和资源。
// SumElements是基准测试名称
BENCHMARK_DEFINE_F(VectorFixture, SumElements)(benchmark::State& state) {
    for (auto _ : state) {
        int sum = 0;
        for (int value : vec) {
            sum += value;
        }
        benchmark::DoNotOptimize(sum);
    }
}

// 使用 BENCHMARK_REGISTER_F 注册测试
// BENCHMARK_REGISTER_F(VectorFixture, SumElements)
//     ->Arg(100)          // 测试 100 个元素
//     ->Arg(1000)         // 测试 1000 个元素  
//     ->Arg(10000);       // 测试 10000 个元素

// 测试 2: BENCHMARK_TEMPLATE_DEFINE_F
/*
    BENCHMARK_TEMPLATE_DEFINE_F 是 Google Benchmark 库中的一个模板化测试定义宏，
    它结合了模板参数和Fixture的功能。

    -   宏的分解解释

        BENCHMARK_TEMPLATE_DEFINE_F(ClassName, TestName, TemplateArg)(benchmark::State& state)

        -   BENCHMARK_TEMPLATE：表示这是一个模板化的基准测试
        -   _DEFINE_F：表示使用Fixture来定义测试
        -   ClassName：Fixture类的名称
        -   TestName：测试的名称
        -   TemplateArg：模板参数
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

struct tas_lock 
{
    std::atomic<bool> lock_ = {false};

    void lock() 
    { 
        while(lock_.exchange(true, std::memory_order_acquire)); 
    }

    void unlock() 
    { 
        lock_.store(false, std::memory_order_release); 
    }
};


struct ttas_lock {
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
            {
                // 可选：减少CPU占用
                #ifdef __x86_64__
                __builtin_ia32_pause();
                #elif defined(__aarch64__)
                asm volatile("yield" ::: "memory");
                #else
                std::this_thread::yield();
                #endif
            }
        }            
    }

    void unlock() 
    { 
        lock_.store(false, std::memory_order_release); 
    }
};

class SpinLock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // 可选：减少CPU占用
            #ifdef __x86_64__
            __builtin_ia32_pause();
            #elif defined(__aarch64__)
            asm volatile("yield" ::: "memory");
            #else
            std::this_thread::yield();
            #endif
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }

    bool try_lock() {
        return !flag.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
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

BENCHMARK_TEMPLATE_DEFINE_F(LockBenchmark, SpinLockTasTest, tas_lock)(benchmark::State& state) {
    for (auto _ : state) {
        lock.lock();
        counter++;
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE_DEFINE_F(LockBenchmark, SpinLockTTasTest, ttas_lock)(benchmark::State& state) {
    for (auto _ : state) {
        lock.lock();
        counter++;
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}

/*
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

BENCHMARK_REGISTER_F(LockBenchmark, SpinLockTasTest)->Name("SpinLockTas")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads

*/
BENCHMARK_REGISTER_F(LockBenchmark, SpinLockTTasTest)->Name("SpinLockTTas")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads
/*
BENCHMARK_REGISTER_F(LockFreeBenchmark, LockFreeTest)->Name("LockFreeTest")
    ->Threads(1)  // Test with 1 thread
    ->Threads(2)  // Test with 2 threads
    ->Threads(4);  // Test with 4 threads
*/

BENCHMARK_MAIN();
