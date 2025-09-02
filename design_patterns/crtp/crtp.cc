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
        // int sum = 0;
        // for (int i = 0; i < 1000000; i++)
        // {
        //     sum += 1;
        // }

        // benchmark::DoNotOptimize(sum);
        // benchmark::ClobberMemory();

        // return sum;
    }
};

// ==================== Benchmark for Virtual Calls ====================
static void BM_VirtualFunction(benchmark::State& state) {
    VirtualDerived d;
    VirtualBase* base_ptr = &d; // Use base pointer to force virtual call
    
    for (auto _ : state) 
    {
        // The core loop: call the virtual function and use the result
        int result = base_ptr->execute(1);
        benchmark::DoNotOptimize(result); // Prevent optimization
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_VirtualFunction);

// ==================== CRTP Interface ====================
template <typename Derived>
class CrtpBase {
public:
    __attribute__((noinline)) int execute(int x) {
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
        // int sum = 0;
        // for (int i = 0; i < 1000000; i++)
        // {
        //     sum += 1;
        // }

        // benchmark::DoNotOptimize(sum);
        // benchmark::ClobberMemory();

        // return sum;
    }
};

// ==================== Benchmark for CRTP Calls ====================
static void BM_CRTPFunction(benchmark::State& state) 
{
    CrtpDerived d;
    // Note: We are using the object by value or direct reference.
    // The key is that the call is resolved statically at compile time.

    for (auto _ : state)
    {
        // The core loop: call the CRTP interface function
        int result = d.execute(1); // This is a static dispatch!
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
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
        for (long long i = 0; i < N; ++i) 
        {
            mOrderSender.SendOrder(); // 这里避免了虚函数开销            
        }
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
        for (long long i = 0; i < N; ++i)
        {
            sender->SendOrder(); // 虚函数开销            
        }
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
