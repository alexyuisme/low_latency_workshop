#include <benchmark/benchmark.h>
#include <string>
#include <vector>
#include <cmath>
#include <random>

using namespace std;

// Note
/*
    -   Clang optimizer advantage

        Clang has more a aggressive optimizer which will translate the 
        following for loop:

        for (int i = 0; i < n; ++i) {
            result += i;
        }

        into a single statement:

        result = n * (n-1) / 2;

        Therefore, using benchmark::DoNotOptimize is necessary 
        to avoid the optimization. 

        Therefore, you will see big differences between 
        gcc and clang in terms of benchmakr performance.
*/

// Benchmark function without loop unrolling
static void BM_LoopWithoutUnrolling(benchmark::State& state) 
{
    for (auto _ : state) 
    {
        int result = 0;
        for (int i = 0; i < state.range(0); ++i) 
        {
            benchmark::DoNotOptimize(result += i);
        }
    }
}
//BENCHMARK(BM_LoopWithoutUnrolling)->Arg(1000);

// Benchmark function with loop unrolling
static void BM_LoopWithUnrolling(benchmark::State& state) 
{
    for (auto _ : state) 
    {
        int result = 0;
        /*
        for (int i = 0; i < state.range(0); i += 4) {
            benchmark::DoNotOptimize(result += i + (i + 1) + (i + 2) + (i + 3));
        }
        */

        /*
        for (int i = 0; i < state.range(0); i += 5) {
            benchmark::DoNotOptimize(result += i + (i + 1) + (i + 2) + (i + 3) + (i + 4));
        }
        */

        for (int i = 0; i < state.range(0); i += 8) 
        {
            benchmark::DoNotOptimize(result += i + (i + 1) + (i + 2) + (i + 3) \ 
                                            + (i + 4) + (i + 5) + (i + 6) + (i + 7));
        }

        // benchmark::DoNotOptimize(result = state.range(0) * (state.range(0) - 1) / 2);

    }
}
//BENCHMARK(BM_LoopWithUnrolling)->Arg(1000);


class LoopFissionFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        n = state.range(0);
        
        // 初始化测试数据
        a.resize(n);
        b.resize(n);
        c.resize(n);
        d.resize(n);
        result1.resize(n);
        result2.resize(n);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(1.0, 100.0);
        
        for (int i = 0; i < n; i++) {
            a[i] = dist(gen);
            b[i] = dist(gen);
            c[i] = dist(gen);
            d[i] = dist(gen);
        }
    }
    
    void TearDown(const benchmark::State&) override {
        // 清理工作
        a.clear();
        b.clear();
        c.clear();
        d.clear();
        result1.clear();
        result2.clear();
    }
    
protected:
    int n;
    std::vector<double> a, b, c, d;
    std::vector<double> result1, result2;
};

// 示例1: 数学计算密集型
BENCHMARK_DEFINE_F(LoopFissionFixture, MathOperations_Fused)(benchmark::State& state) {
    for (auto _ : state) {
        // 融合循环 - 混合不同类型的数学操作
        for (int i = 0; i < n; i++) {
            result1[i] = std::sin(a[i]) + std::cos(b[i]);    // 三角函数
            result2[i] = std::log(c[i] + 1.0) * std::sqrt(d[i]); // 对数和平方根
        }
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(LoopFissionFixture, MathOperations_Fission)(benchmark::State& state) {
    for (auto _ : state) {
        // 分裂循环 - 分离不同类型的数学操作
        for (int i = 0; i < n; i++) {
            result1[i] = std::sin(a[i]) + std::cos(b[i]);  // 只做三角函数
        }
        for (int i = 0; i < n; i++) {
            result2[i] = std::log(c[i] + 1.0) * std::sqrt(d[i]); // 只做对数和平方根
        }
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
        benchmark::ClobberMemory();
    }
}

// 示例2: 内存访问模式优化
BENCHMARK_DEFINE_F(LoopFissionFixture, MemoryAccess_Fused)(benchmark:: State& state) {
    for (auto _ : state) {
        // 融合循环 - 交替访问不同数组
        for (int i = 0; i < n; i++) {
            a[i] = b[i] * 2.0 + c[i];  // 访问数组b和c
            d[i] = a[i] + b[i] * 0.5;  // 再次访问b，混合访问模式
        }
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(d);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(LoopFissionFixture, MemoryAccess_Fission)(benchmark::State& state) {
    for (auto _ : state) {
        // 分裂循环 - 每个循环专注访问相关数据
        for (int i = 0; i < n; i++) {
            a[i] = b[i] * 2.0 + c[i];  // 主要访问b和c
        }
        for (int i = 0; i < n; i++) {
            d[i] = a[i] + b[i] * 0.5;  // 访问a和b
        }
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(d);
        benchmark::ClobberMemory();
    }
}

// 示例3: 简单和复杂操作混合
BENCHMARK_DEFINE_F(LoopFissionFixture, MixedComplexity_Fused)(benchmark::State& state) {
    for (auto _ : state) {
        // 融合循环 - 简单和复杂操作混合
        for (int i = 0; i < n; i++) {
            result1[i] = a[i] + b[i];                    // 简单加法
            result2[i] = std::exp(std::sin(c[i]));       // 复杂数学函数
        }
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(LoopFissionFixture, MixedComplexity_Fission)(benchmark::State& state) {
    for (auto _ : state) {
        // 分裂循环 - 分离简单和复杂操作
        for (int i = 0; i < n; i++) {
            result1[i] = a[i] + b[i];  // 只做简单操作
        }
        for (int i = 0; i < n; i++) {
            result2[i] = std::exp(std::sin(c[i]));  // 只做复杂操作
        }
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
        benchmark::ClobberMemory();
    }
}

// 示例4: 条件检查分离
BENCHMARK_DEFINE_F(LoopFissionFixture, ConditionCheck_Fused)(benchmark::State& state) {
    for (auto _ : state) {
        // 融合循环 - 条件检查和计算混合
        for (int i = 0; i < n; i++) {
            if (a[i] > 50.0) {  // 条件检查
                result1[i] = std::sqrt(a[i]) * std::log(b[i]);  // 复杂计算
            } else {
                result1[i] = 0.0;
            }
        }
        benchmark::DoNotOptimize(result1);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(LoopFissionFixture, ConditionCheck_Fission)(benchmark::State& state) {
    for (auto _ : state) {
        // 分裂循环 - 先做条件检查，再做计算
        std::vector<bool> should_compute(n);
        for (int i = 0; i < n; i++) {
            should_compute[i] = (a[i] > 50.0);  // 只做条件检查
        }
        for (int i = 0; i < n; i++) {
            if (should_compute[i]) {
                result1[i] = std::sqrt(a[i]) * std::log(b[i]);  // 只做复杂计算
            } else {
                result1[i] = 0.0;
            }
        }
        benchmark::DoNotOptimize(result1);
        benchmark::ClobberMemory();
    }
}

// 注册基准测试
BENCHMARK_REGISTER_F(LoopFissionFixture, MathOperations_Fused)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, MathOperations_Fission)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, MemoryAccess_Fused)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, MemoryAccess_Fission)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, MixedComplexity_Fused)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, MixedComplexity_Fission)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, ConditionCheck_Fused)->Range(1024, 256*1024);
BENCHMARK_REGISTER_F(LoopFissionFixture, ConditionCheck_Fission)->Range(1024, 256*1024);

// 简单函数版本（不使用Fixture）
static void SimpleLoopFission_Benchmark(benchmark::State& state) {
    int n = state.range(0);
    std::vector<double> input(n), output1(n), output2(n);
    
    // 初始化
    for (int i = 0; i < n; i++) {
        input[i] = i * 0.1;
    }
    
    for (auto _ : state) {
        // 测试不同的循环策略
        if (state.range(1) == 0) {
            // 融合循环
            for (int i = 0; i < n; i++) {
                output1[i] = std::sin(input[i]);
                output2[i] = std::cos(input[i]);
            }
        } else {
            // 分裂循环
            for (int i = 0; i < n; i++) {
                output1[i] = std::sin(input[i]);
            }
            for (int i = 0; i < n; i++) {
                output2[i] = std::cos(input[i]);
            }
        }
        benchmark::DoNotOptimize(output1);
        benchmark::DoNotOptimize(output2);
    }
}

BENCHMARK(SimpleLoopFission_Benchmark)
    ->Args({1024, 0})    // 融合, 1024元素
    ->Args({1024, 1})    // 分裂, 1024元素  
    ->Args({8192, 0})    // 融合, 8192元素
    ->Args({8192, 1})    // 分裂, 8192元素
    ->Args({65536, 0})   // 融合, 65536元素
    ->Args({65536, 1});  // 分裂, 65536元素

BENCHMARK_MAIN();
