#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <iostream>
#include <thread>
#include <execution>
#include <immintrin.h>  // AVX/AVX2 intrinsics

namespace bm = benchmark;

static void i32_addition(benchmark::State& state)
{
    int32_t a, b, c;
    for (auto _ : state)
        c = a + b;
}
//BENCHMARK(i32_addition);

static void i32_addition_random(bm::State &state) {
    int32_t c = 0;
    for (auto _ : state)
        c = std::rand() + std::rand();
}
//BENCHMARK(i32_addition_random)->Threads(8);

static void i32_addition_semi_random(bm::State &state) {
    int32_t a = std::rand(), b = std::rand(), c = 0;
    for (auto _ : state)
        bm::DoNotOptimize(c = (++a) + (++b));
}
//BENCHMARK(i32_addition_semi_random)->Threads(8);

static void f64_sin(bm::State &state) {
    double argument = std::rand(), result = 0;
    for (auto _ : state)
        bm::DoNotOptimize(result = std::sin(argument += 1.0));
}
//BENCHMARK(f64_sin);

// why slower than f64_sin?
static void f64_sin_maclaurin(bm::State &state) {
    double argument = std::rand(), result = 0;
    for (auto _ : state) {
        argument += 1.0;
        result = argument - std::pow(argument, 3) / 6 + std::pow(argument, 5) / 120;
        bm::DoNotOptimize(result);
    }
}
//BENCHMARK(f64_sin_maclaurin);

__attribute__((optimize("-ffast-math")))
static void f64_sin_maclaurin_powless(bm::State &state) {
    double argument = std::rand(), result = 0;
    for (auto _ : state) {
        argument += 1.0;
        result = (argument) - (argument * argument * argument) / 6.0 +
                 (argument * argument * argument * argument * argument) / 120.0;
        bm::DoNotOptimize(result);
    }
}
//BENCHMARK(f64_sin_maclaurin_powless);

static void i64_division_by_const(bm::State& state)
{
    int64_t money = 2147483647;
    int64_t a = std::rand(), c;
    for (auto _ : state)
        bm::DoNotOptimize(c = (++a) / *std::launder(&money));
}
//BENCHMARK(i64_division_by_const);

static void i64_division_by_constexpr(bm::State &state) {
    constexpr int64_t b = 2147483647;
    int64_t a = std::rand(), c;
    for (auto _ : state)
        bm::DoNotOptimize(c = (++a) / b);
}
//BENCHMARK(i64_division_by_constexpr);

// Hardware Acceleration without Intrinsics
[[gnu::target("default")]]static void u64_population_count(bm::State &state) {
    auto a = static_cast<uint64_t>(std::rand());
    for (auto _ : state)
        bm::DoNotOptimize(__builtin_popcount(++a));
}
//BENCHMARK(u64_population_count);

[[gnu::target("popcnt")]] static void u64_population_count_x86(bm::State &state) {
    auto a = static_cast<uint64_t>(std::rand());
    for (auto _ : state)
        bm::DoNotOptimize(__builtin_popcount(++a));
}
//BENCHMARK(u64_population_count_x86);

// Broader Logic and Memory
// Data Alignment

/*
    Compute may be expensive, but memory accesses always are! The more you 
    miss your CPU caches, the more you waste time!
*/
constexpr size_t f32s_in_cache_line_k = 64 / sizeof(float); // 16 floats
constexpr size_t f32s_in_cache_line_half_k = f32s_in_cache_line_k / 2; // 8 floats

struct alignas(64) f32_array_t {
    float raw[f32s_in_cache_line_k * 2]; // 32 floats
};

// Let’s illustrate it by creating a cache-aligned array with 32x floats. That
// means 2x 64-byte cache lines worth of content.
static void f32_pairwise_accumulation(bm::State &state) {
    f32_array_t a, b, c;
    for (auto _ : state)
        for (size_t i = f32s_in_cache_line_half_k; i != f32s_in_cache_line_half_k * 2; ++i)
            bm::DoNotOptimize(c.raw[i] = a.raw[i] + b.raw[i]);
}
//BENCHMARK(f32_pairwise_accumulation);

static void f32_pairwise_accumulation_aligned(bm::State &state) {
    f32_array_t a, b, c;
    for (auto _ : state)
        for (size_t i = 0; i != f32s_in_cache_line_half_k; ++i)
            bm::DoNotOptimize(c.raw[i] = a.raw[i] + b.raw[i]);
}
//BENCHMARK(f32_pairwise_accumulation_aligned);

static void cost_of_branching_for_different_depth(bm::State& state)
{
    auto count = static_cast<size_t>(state.range(0));
    std::vector<int32_t> random_values(count);
    std::generate_n(random_values.begin(), random_values.size(), &std::rand);
    int32_t variable = 0;
    size_t iteration = 0;

    for (auto _ : state) {
        // Loop around to avoid out-of-bound access.
        // For power-of-two sizes of `random_values` the `(++iteration) & (count - 1)`
        // is identical to `(++iteration) % count`.
        int32_t random = random_values[(++iteration) & (count - 1)]; 
        bm::DoNotOptimize(variable = (random & 1) ? (variable + random) : (variable * random));
    }
}

//BENCHMARK(cost_of_branching_for_different_depth)->RangeMultiplier(4)->Range(256, 32 * 1024);

static void upper_cost_of_pausing(bm::State &state) {
    int32_t a = std::rand(), c = 0;
    for (auto _ : state) {
        //state.PauseTiming();
        ++a;
        //state.ResumeTiming();
        bm::DoNotOptimize(c += a);
    }
}
//BENCHMARK(upper_cost_of_pausing);

template <typename execution_policy_t>
static void super_sort(bm::State &state, execution_policy_t &&policy) {

    auto count = static_cast<size_t>(state.range(0));
    std::vector<int32_t> array(count);
    std::iota(array.begin(), array.end(), 1);

    for (auto _ : state) {
        std::reverse(policy, array.begin(), array.end());
        std::sort(policy, array.begin(), array.end());
        bm::DoNotOptimize(array.size());
    }

    state.SetComplexityN(count);
    state.SetItemsProcessed(count * state.iterations());
    state.SetBytesProcessed(count * state.iterations() * sizeof(int32_t));
}

/*
BENCHMARK_CAPTURE(super_sort, seq, std::execution::seq)
    ->RangeMultiplier(8)
    ->Range(1l << 20, 1l << 32)
    ->MinTime(10)
    ->Complexity(bm::oNLogN);

BENCHMARK_CAPTURE(super_sort, par_unseq, std::execution::par_unseq)
    ->RangeMultiplier(8)
    ->Range(1l << 20, 1l << 32)
    ->MinTime(10)
    ->Complexity(bm::oNLogN);
*/

// Test string copy performance
static void BM_StringCopy(benchmark::State& state) 
{
    std::string x = "hello";
    for (auto _ : state) 
    {
        // -O3 will not optimize away the copy variable
        std::string copy(x);
        //benchmark::ClobberMemory(); // Force memory operations to be flushed
        //benchmark::DoNotOptimize(copy);
    }
}
//BENCHMARK(BM_StringCopy);

// A pure computation function with no side effects
int pure_computation(int x) {
    return x * x + 2 * x + 1;
}

static void BM_PureComputation(benchmark::State& state) {
    int result = 0;
    for (auto _ : state) {
        result = pure_computation(42);
        // Without ClobberMemory, the compiler might optimize away the entire computation!
        benchmark::ClobberMemory(); // Force the compiler to actually execute the computation
    }
    benchmark::DoNotOptimize(result);
}
// BENCHMARK(BM_PureComputation);





#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>

// Base class with virtual function
class BaseVirtual {
public:
    virtual int compute(int x) = 0;
    virtual ~BaseVirtual() = default;
};

// Derived class implementation
class DerivedVirtual : public BaseVirtual {
public:
    int compute(int x) override {
        return x * 2 + 1;  // Some computation
    }
};

// Regular class without virtual functions
class Regular {
public:
    int compute(int x) {
        return x * 2 + 1;  // Same computation
    }
};

// Template version for comparison
template<typename T>
class TemplateClass {
public:
    int compute(int x) {
        return x * 2 + 1;
    }
};

// Static benchmark functions
static void BM_RegularFunction(benchmark::State& state) {
    Regular obj;
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj.compute(state.iterations()));
    }
}

static void BM_VirtualFunction(benchmark:: State& state) {
    std::unique_ptr<BaseVirtual> obj = std::make_unique<DerivedVirtual>();
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj->compute(state.iterations()));
    }
}

static void BM_TemplateFunction(benchmark::State& state) {
    TemplateClass<int> obj;
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj.compute(state.iterations()));
    }
}

// Benchmark with multiple objects in array
static void BM_VirtualArray(benchmark::State& state) {
    const int size = 1000;
    std::vector<std::unique_ptr<BaseVirtual>> objects;
    objects.reserve(size);
    for (int i = 0; i < size; ++i) {
        objects.push_back(std::make_unique<DerivedVirtual>());
    }
    
    int result = 0;
    for (auto _ : state) {
        for (int i = 0; i < size; ++i) {
            benchmark::DoNotOptimize(result += objects[i]->compute(i));
        }
    }
}

static void BM_RegularArray(benchmark::State& state) {
    const int size = 1000;
    std::vector<Regular> objects(size);
    
    int result = 0;
    for (auto _ : state) {
        for (int i = 0; i < size; ++i) {
            benchmark::DoNotOptimize(result += objects[i].compute(i));
        }
    }
}

// Register benchmarks
BENCHMARK(BM_RegularFunction);
BENCHMARK(BM_VirtualFunction);
BENCHMARK(BM_TemplateFunction);
BENCHMARK(BM_RegularArray);
BENCHMARK(BM_VirtualArray);

// Benchmark with branch prediction impact
static void BM_MixedVirtualCalls(benchmark::State& state) {
    std::vector<std::unique_ptr<BaseVirtual>> objects;
    const int size = 1000;
    
    // Create mixed types (though same implementation for fairness)
    for (int i = 0; i < size; ++i) {
        objects.push_back(std::make_unique<DerivedVirtual>());
    }
    
    // Shuffle to break potential pattern
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(objects.begin(), objects.end(), g);
    
    int result = 0;
    for (auto _ : state) {
        for (int i = 0; i < size; ++i) {
            benchmark::DoNotOptimize(result += objects[i]->compute(i));
        }
    }
}

static void BM_RegularInlined(benchmark::State& state) {
    class LocalRegular {
    public:
        __attribute__((always_inline)) int compute(int x) {
            return x * 2 + 1;
        }
    };
    
    LocalRegular obj;
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj.compute(state.iterations()));
    }
}

BENCHMARK(BM_MixedVirtualCalls);
BENCHMARK(BM_RegularInlined);

struct Foo {
    int data[16];
    Foo() { for (int i = 0; i < 16; ++i) data[i] = i; }
};

static void BM_UniquePtr_CreateDestroy(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::make_unique<Foo>();
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_UniquePtr_CreateDestroy)->Threads(1);

static void BM_UniquePtr_Move(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::make_unique<Foo>();
        auto q = std::move(p);
        benchmark::DoNotOptimize(q.get());
    }
}
BENCHMARK(BM_UniquePtr_Move)->Threads(1);

static void BM_SharedPtr_CreateDestroy_New(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::shared_ptr<Foo>(new Foo);
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_SharedPtr_CreateDestroy_New)->Threads(1);

static void BM_SharedPtr_CreateDestroy_MakeShared(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::make_shared<Foo>();
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_SharedPtr_CreateDestroy_MakeShared)->Threads(1);

static void BM_SharedPtr_Copy(benchmark::State& state) {
    auto p = std::make_shared<Foo>();
    for (auto _ : state) {
        auto p2 = p; // atomic increment of refcount when copied
        benchmark::DoNotOptimize(p2.get());
    }
}
BENCHMARK(BM_SharedPtr_Copy)->Threads(1);

// Passing by value vs const-ref
void consume_shared_by_value(std::shared_ptr<Foo> p) { benchmark::DoNotOptimize(p.get()); }
void consume_shared_by_ref(const std::shared_ptr<Foo>& p) { benchmark::DoNotOptimize(p.get()); }

static void BM_SharedPtr_PassByValue(benchmark::State& state) {
    auto p = std::make_shared<Foo>();
    for (auto _ : state) consume_shared_by_value(p);
}
BENCHMARK(BM_SharedPtr_PassByValue)->Threads(1);

static void BM_SharedPtr_PassByRef(benchmark::State& state) {
    auto p = std::make_shared<Foo>();
    for (auto _ : state) consume_shared_by_ref(p);
}
BENCHMARK(BM_SharedPtr_PassByRef)->Threads(1);

#include <benchmark/benchmark.h>
#include <vector>
#include <random>

// 初始化测试数据（确保不成为性能瓶颈）
std::vector<float> setup_data(size_t size) {
    std::vector<float> data(size);
    std::mt19937 gen(42); 
    std::uniform_real_distribution<float> dis(1.0f, 2.0f);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dis(gen);
    }
    return data;
}

// 需要测试的 For 循环（向量加法）
static void BM_SimdLoop(benchmark::State& state) {
    const size_t N = state.range(0);
    auto src1 = setup_data(N);
    auto src2 = setup_data(N);
    std::vector<float> dest(N, 0.0f);

    // 获取裸指针，有助于编译器识别并进行 SIMD 优化
    float* p_src1 = src1.data();
    float* p_src2 = src2.data();
    float* p_dest = dest.data();

    for (auto _ : state) {
        // 核心循环：测试 SIMD 自动向量化
        // 使用 __builtin_assume_aligned 或 restrict 关键字（可选）能进一步帮助编译器
        //#pragma omp simd // 提示编译器尝试 SIMD 化（如果开启了 OpenMP）
        for (size_t i = 0; i < N; ++i) {
            p_dest[i] = p_src1[i] * 2.0f + p_src2[i];
        }
        
        // 关键：防止编译器将 dest 的计算当做死代码直接优化删除
        benchmark::DoNotOptimize(p_dest);
        benchmark::ClobberMemory();
    }

    // 计算吞吐量 (可选)
    state.SetBytesProcessed(int64_t(state.iterations()) * N * sizeof(float) * 3);
}

// 设定测试规模：从 1024 到 1048576（1M）个 float 元素
BENCHMARK(BM_SimdLoop)->RangeMultiplier(4)->Range(1024, 1024 * 1024);

// 方案3：更激进的优化 - 循环展开 + 寄存器重用
static void BM_SimdLoop_Intrinsic_Unrolled(benchmark::State& state) {
    const size_t N = state.range(0);
    auto src1 = setup_data(N);
    auto src2 = setup_data(N);
    std::vector<float> dest(N, 0.0f);

    float* p_src1 = src1.data();
    float* p_src2 = src2.data();
    float* p_dest = dest.data();
    
    __m256 vec_two = _mm256_set1_ps(2.0f);

    for (auto _ : state) {
        size_t i = 0;

        // 一次处理 32 个元素（4组，每组8个）
        // 减少循环控制开销，增加指令级并行
        for (; i + 31 < N; i += 32) {
            // 第1组
            __m256 v1_src1 = _mm256_loadu_ps(p_src1 + i);
            __m256 v1_src2 = _mm256_loadu_ps(p_src2 + i);
            __m256 v1_res = _mm256_fmadd_ps(v1_src1, vec_two, v1_src2);
            _mm256_storeu_ps(p_dest + i, v1_res);
            
            // 第2组
            __m256 v2_src1 = _mm256_loadu_ps(p_src1 + i + 8);
            __m256 v2_src2 = _mm256_loadu_ps(p_src2 + i + 8);
            __m256 v2_res = _mm256_fmadd_ps(v2_src1, vec_two, v2_src2);
            _mm256_storeu_ps(p_dest + i + 8, v2_res);
            
            // 第3组
            __m256 v3_src1 = _mm256_loadu_ps(p_src1 + i + 16);
            __m256 v3_src2 = _mm256_loadu_ps(p_src2 + i + 16);
            __m256 v3_res = _mm256_fmadd_ps(v3_src1, vec_two, v3_src2);
            _mm256_storeu_ps(p_dest + i + 16, v3_res);
            
            // 第4组
            __m256 v4_src1 = _mm256_loadu_ps(p_src1 + i + 24);
            __m256 v4_src2 = _mm256_loadu_ps(p_src2 + i + 24);
            __m256 v4_res = _mm256_fmadd_ps(v4_src1, vec_two, v4_src2);
            _mm256_storeu_ps(p_dest + i + 24, v4_res);
        }

        // 处理剩余元素
        for (; i < N; ++i) {
            p_dest[i] = p_src1[i] * 2.0f + p_src2[i];
        }
        
        benchmark::DoNotOptimize(p_dest);
        benchmark::ClobberMemory();
    }

    // 计算吞吐量 (可选)
    state.SetBytesProcessed(int64_t(state.iterations()) * N * sizeof(float) * 3);
}
// 设定测试规模：从 1024 到 1048576（1M）个 float 元素
BENCHMARK(BM_SimdLoop_Intrinsic_Unrolled)->RangeMultiplier(4)->Range(1024, 1024 * 1024);



//  BM_Superscalar_Bottleneck vs. BM_Superscalar_Parallel
//  BM_Superscalar_Parallel should be 4 times faster than BM_Superscalar_Bottleneck

// 1. LATENCY BOUND (Scalar Bottleneck)
// The CPU has multiple multipliers, but each step MUST wait for the previous 
// multiplication to completely finish (typically 3-5 clock cycles).
static void BM_Superscalar_Bottleneck(benchmark::State& state) {
    for (auto _ : state) {
        double a = 1.01;
        // A single long chain of execution
        for (int i = 0; i < 10000; ++i) {
            a = a * 1.01; // "a" depends on the previous "a", therefore no simd vectorization 
        }
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_Superscalar_Bottleneck);

// 2. LATENCY HIDDEN (Superscalar Friendly)
// We break the dependency into 4 independent streams. The compiler cannot 
// vectorize this with SIMD easily, but the superscalar hardware can execute 
// 4 multiplications simultaneously across 4 different ALU pipelines.
static void BM_Superscalar_Parallel(benchmark::State& state) {
    for (auto _ : state) {
        double a = 1.01;
        double b = 1.01;
        double c = 1.01;
        double d = 1.01;

        for (int i = 0; i < 2500; ++i) { // 2500 * 4 = 10000 operations total
            // a, b, c, d are independent on each other. Therefore, parallel will kick in
            a = a * 1.01; // "a" depends on the previous "a", therefore no simd vectorization
            b = b * 1.01; // "b" depends on the previous "b", therefore no simd vectorization
            c = c * 1.01; // "c" depends on the previous "c", therefore no simd vectorization
            d = d * 1.01; // "d" depends on the previous "d", therefore no simd vectorization
        }
        double total = a + b + c + d;
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_Superscalar_Parallel);

template <typename T>
struct AgeWrapper {
    enum class Age : T {};
};

template <typename T>
using Age = typename AgeWrapper<T>::Age;

// The targeted average_age function template
template <typename T>
Age<T> average_age(const std::vector<Age<T>>& ages) {
    int total = 0;
    for (Age<T> age : ages) {
        // !! uint8_t -> int calls 
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
/*
    Optimize type size: Pick smaller primitive types

    -   signed char or short instead of int
    -   float instead of double
    -   specify underlying type of enumerations
*/
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

// 16KN = 16 * 1024 elements
constexpr int KN = 1024;

//  16KN ~ 128KN
// DenseRange(start, end, step) 
BENCHMARK_TEMPLATE(BM_AverageAge, int)     ->Name("BM_Age_Int")  ->DenseRange(16 * KN, 128 * KN, 16 * KN);
BENCHMARK_TEMPLATE(BM_AverageAge, short)   ->Name("BM_Age_Short")->DenseRange(16 * KN, 128 * KN, 16 * KN);
BENCHMARK_TEMPLATE(BM_AverageAge, uint8_t) ->Name("BM_Age_Uint8")->DenseRange(16 * KN, 128 * KN, 16 * KN);

BENCHMARK_MAIN();
