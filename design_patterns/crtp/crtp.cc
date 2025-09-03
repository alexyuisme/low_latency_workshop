#include <benchmark/benchmark.h>
#include <iostream>
#include <array>
#include <memory>

// Note
/*
  It looks both BM_VirtualFunction and BM_CRTPFunction have the same performance. 
  How come? 
*/

// Hypothesis - what makes virtual calls slower
/*
    -   Extra indirection (pointer dereference) for each call to a virtual 
        method.

    -   Virtual methods usually can’t be inlined, which may be a significant 
        cost hit for some small methods.

    -   Additional pointer per object. On 64-bit systems which are prevalent 
        these days, this is 8 bytes per object. For small objects that carry 
        little data this may be a serious overhead.
*/

//
// ==================== Virtual Function Interface ====================
class VirtualBase {
public:
    virtual ~VirtualBase() = default; // Good practice: virtual destructor
    __attribute__((noinline)) virtual int execute(int x) = 0;
};

class VirtualDerived : public VirtualBase {
public:
    __attribute__((noinline)) int execute(int x) override {
        // Some non-trivial work that's easy to optimize away
        return x * x + 2 * x + 1; // A simple quadratic
    }
};

struct ExecuteManagerVirtual
{
    ExecuteManagerVirtual(std::unique_ptr<VirtualBase> e) : executor(std::move(e)) {}

    void MainLoop()
    {        
        volatile long long volatile_N = 200'000'000; // 防止循环次数被优化
        int dummy_result = 0;
        
        for (long long i = 0; i < volatile_N; ++i) {
            executor->execute(1); // This is a dynamic dispatch!
            dummy_result += i; // 累积结果
        }
        // The core loop: call the CRTP interface function
        benchmark::DoNotOptimize(dummy_result);
        benchmark::ClobberMemory();
    }

    std::unique_ptr<VirtualBase> executor;
};

// ==================== Benchmark for Virtual Calls ====================
static void BM_VirtualFunction(benchmark::State& state) {
    // VirtualDerived d;
    // VirtualBase* base_ptr = &d; // Use base pointer to force virtual call
    
    auto manager_v_ptr = std::make_unique<ExecuteManagerVirtual>(std::make_unique<VirtualDerived>());

    for (auto _ : state) 
    {
        // volatile long long volatile_N = 200'000'000; // 防止循环次数被优化
        // int dummy_result = 0;
        
        // for (long long i = 0; i < volatile_N; ++i) {
        //     base_ptr->execute(1); // This is a dynamic dispatch!
        //     dummy_result += i; // 累积结果
        // }
        // // The core loop: call the CRTP interface function
        // benchmark::DoNotOptimize(dummy_result);
        // benchmark::ClobberMemory();
        manager_v_ptr->MainLoop();
    }
}
BENCHMARK(BM_VirtualFunction);

// ==================== CRTP Interface ====================
template <typename Derived>
class CrtpBase {
public:
    int execute(int x) {
        // Static polymorphism: delegate to the derived class
        return static_cast<Derived*>(this)->execute_impl(x);
    }

    // Non-virtual destructor is fine for CRTP
};

class CrtpDerived : public CrtpBase<CrtpDerived> {
public:
    __attribute__((noinline)) int execute_impl(int x) 
    {
        // Perform the *exact same* operation as VirtualDerived
        return x * x + 2 * x + 1;
    }
};

struct ExecuteManagerCrtp
{
    ExecuteManagerCrtp(std::unique_ptr<CrtpDerived> e) : executor(std::move(e)) {}

    void MainLoop()
    {        
        volatile long long volatile_N = 200'000'000; // 防止循环次数被优化
        int dummy_result = 0;
        
        for (long long i = 0; i < volatile_N; ++i) {
            executor->execute(1); // This is a dynamic dispatch!
            dummy_result += i; // 累积结果
        }
        // The core loop: call the CRTP interface function
        benchmark::DoNotOptimize(dummy_result);
        benchmark::ClobberMemory();
    }

    std::unique_ptr<CrtpDerived> executor;
};

// ==================== Benchmark for CRTP Calls ====================
static void BM_CRTPFunction(benchmark::State& state) 
{
    // CrtpDerived d;
    // CrtpBase<CrtpDerived>* base_ptr = &d;

    auto manager_crtp_ptr = std::make_unique<ExecuteManagerCrtp>(std::make_unique<CrtpDerived>());

    for (auto _ : state)
    {
        // volatile long long volatile_N = 200'000'000; // 防止循环次数被优化
        // int dummy_result = 0;
        
        // for (long long i = 0; i < volatile_N; ++i) {
        //     base_ptr->execute(1); // This is a static dispatch!
        //     dummy_result += i; // 累积结果
        // }
        // // The core loop: call the CRTP interface function
        // benchmark::DoNotOptimize(dummy_result);
        // benchmark::ClobberMemory();
        manager_crtp_ptr->MainLoop();
    }
}
BENCHMARK(BM_CRTPFunction);

// 模拟配置文件
struct Config 
{
    bool use_a = true;
    bool UseOrderSenderA() const { return use_a; }
};

constexpr long long N = 200'000'000;  // 调用次数

// ———— 第一种：模板+静态分发 ————
struct OrderSenderA 
{
    void SendOrder() { /* 模拟空操作 */ }
};

struct OrderSenderB 
{
    void SendOrder() { /* 模拟空操作 */ }
};

struct IOrderManagerTpl 
{
    virtual void MainLoop() = 0;
    virtual ~IOrderManagerTpl() = default;
};

template <typename T>
struct OrderManagerTpl : IOrderManagerTpl 
{
    void MainLoop() final 
    {        
        volatile long long volatile_N = N; // 防止循环次数被优化
        int dummy_result = 0;
        
        for (long long i = 0; i < volatile_N; ++i) {
            mOrderSender.SendOrder(); // 假设返回int
            dummy_result += i; // 累积结果
        }
        
        // 双重保护：防止整个循环被优化
        benchmark::DoNotOptimize(dummy_result);
        benchmark::ClobberMemory();
    }

    T mOrderSender;
};

std::unique_ptr<IOrderManagerTpl> MakeTpl(const Config& c) 
{
    if (c.UseOrderSenderA())
        return std::make_unique<OrderManagerTpl<OrderSenderA>>();
    else
        return std::make_unique<OrderManagerTpl<OrderSenderB>>();
}

// ———— 第二种：纯虚函数+动态分发 ————
struct IOrderSender 
{
    virtual void SendOrder() = 0;
    virtual ~IOrderSender() = default;
};

struct OrderSenderA_V : IOrderSender 
{
    void SendOrder() override { /* 模拟空操作 */ }
};

struct OrderSenderB_V : IOrderSender 
{
    void SendOrder() override { /* 模拟空操作 */ }
};

struct OrderManagerV 
{
    OrderManagerV(std::unique_ptr<IOrderSender> s) : sender(std::move(s)) {}
    void MainLoop() 
    {
        volatile long long volatile_N = N; // 防止循环次数被优化
        int dummy_result = 0;
        for (long long i = 0; i < volatile_N; ++i)
        {
            sender->SendOrder(); // 虚函数开销
            dummy_result += i;
        }

        // 双重保护：防止整个循环被优化
        benchmark::DoNotOptimize(dummy_result);
        benchmark::ClobberMemory();
    }
    
    std::unique_ptr<IOrderSender> sender;
};

std::unique_ptr<OrderManagerV> MakeVirtual(const Config& c) 
{
    if (c.UseOrderSenderA())
        return std::make_unique<OrderManagerV>(std::make_unique<OrderSenderA_V>());
    else
        return std::make_unique<OrderManagerV>(std::make_unique<OrderSenderB_V>());
}

// ==================== Benchmark for Template Static Calls ====================
static void BM_TemplateCalls(benchmark::State& state) 
{
    Config cfg;
    // 1) 测模板写法速度
    cfg.use_a = true;
    auto mTpl = MakeTpl(cfg);

    // Note: We are using the object by value or direct reference.
    // The key is that the call is resolved statically at compile time.

    for (auto _ : state)
    {
        mTpl->MainLoop();
    }
}
// BENCHMARK(BM_TemplateCalls);

// ==================== Benchmark for Template Static Calls ====================
static void BM_VirtualCalls(benchmark::State& state) 
{
    Config cfg;
    // 1) 测模板写法速度
    cfg.use_a = true;
    auto mV = MakeVirtual(cfg);

    // Note: We are using the object by value or direct reference.
    // The key is that the call is resolved statically at compile time.

    for (auto _ : state)
    {
        mV->MainLoop();
    }
}
// BENCHMARK(BM_VirtualCalls);

// Main macro for the benchmark
BENCHMARK_MAIN();
