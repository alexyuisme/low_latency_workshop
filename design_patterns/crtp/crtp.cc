#include <benchmark/benchmark.h>
#include <iostream>

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
    virtual int execute(int x) = 0;
};

class VirtualDerived : public VirtualBase {
public:
    __attribute__((noinline)) int execute(int x) override {
        // Some non-trivial work that's easy to optimize away
        return x * x + 2 * x + 1; // A simple quadratic
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
    int execute(int x) {
        // Static polymorphism: delegate to the derived class
        return static_cast<Derived*>(this)->execute_impl(x);
    }

    // Non-virtual destructor is fine for CRTP
};

class CrtpDerived : public CrtpBase<CrtpDerived> {
public:
    inline int execute_impl(int x) 
    {
        // Perform the *exact same* operation as VirtualDerived
        return x * x + 2 * x + 1;
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

/*
class DynamicInterface {
public:
    virtual void tick(uint64_t n) = 0;
    virtual uint64_t getvalue() = 0;
};

class DynamicImplementation : public DynamicInterface 
{
    uint64_t counter;

    public:
    DynamicImplementation()
        : counter(0) 
    {
    }

    virtual void tick(uint64_t n) 
    {
        counter += n;
    }

    virtual uint64_t getvalue() 
    {
        return counter;
    }
};

const unsigned N = 40000;

void run_dynamic(DynamicInterface* obj) 
{
    for (unsigned i = 0; i < N; ++i) 
    {
        for (unsigned j = 0; j < i; ++j) 
        {
            obj->tick(j);
        }
    }
}

static void BM_Dynamic(benchmark::State& state) 
{

    for (auto _ : state) 
    {
        DynamicImplementation obj;
        run_dynamic(&obj);
        benchmark::DoNotOptimize(obj); 
    }
    benchmark::ClobberMemory();
}
// BENCHMARK(BM_Dynamic);

template <typename Implementation>
class CRTPInterface {
public:
  void tick(uint64_t n) {
    impl().tick(n);
  }

  uint64_t getvalue() {
    return impl().getvalue();
  }
private:
  Implementation& impl() {
    return *static_cast<Implementation*>(this);
  }
};

class CRTPImplementation : public CRTPInterface<CRTPImplementation> {
  uint64_t counter;
public:
  CRTPImplementation()
    : counter(0) {
  }

  void tick(uint64_t n) {
    counter += n;
  }

  uint64_t getvalue() {
    return counter;
  }
};

template <typename Implementation>
void run_crtp(CRTPInterface<Implementation>* obj) {
  for (unsigned i = 0; i < N; ++i) {
    for (unsigned j = 0; j < i; ++j) {
      obj->tick(j);
    }
  }
}

static void BM_CRTP(benchmark::State& state) 
{

    for (auto _ : state) 
    {
        CRTPImplementation obj;
        run_crtp(&obj);
        benchmark::DoNotOptimize(obj); 
    }
    benchmark::ClobberMemory();
}
// BENCHMARK(BM_CRTP);
*/


// Main macro for the benchmark
BENCHMARK_MAIN();
