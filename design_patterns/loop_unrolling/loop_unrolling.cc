#include <benchmark/benchmark.h>
#include <string>
#include <vector>
using namespace std;

#include <benchmark/benchmark.h>

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
BENCHMARK(BM_LoopWithoutUnrolling)->Arg(1000);

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
BENCHMARK(BM_LoopWithUnrolling)->Arg(1000);

BENCHMARK_MAIN();