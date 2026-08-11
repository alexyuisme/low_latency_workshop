#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cstring> // for memcpy

// Note: 
/*
    With the compiler optimization flag -O3 enabled (e.g. -O3), NoPrefetch() is 
    actually running faster than WithPrefetch(). That happens because NoPrefetch() 
    uses auto-vectorization but in WithPrefetch(), the introduction of 
    __builtin_prefetch disables the usage of auto-vectorization.

    -   Auto-Vectorization

        Compiler Auto-Vectorization is Disrupted (The Primary Reason) At the -O3 
        optimization level, the compiler attempts to perform auto-vectorization. 
        For simple loops like yours that sum a contiguous array, the compiler is 
        highly adept at optimizing them using SIMD instructions (such as SSE or 
        AVX).

    -   Drawbacks of __bulitin_prefetch

        -   Compiler optimization is hindered: 
        
            __builtin_prefetch is a complex instruction with side effects. It 
            makes the loop body more complex and irregular.

        -   Vectorization is prevented: The compiler is usually unable to 
            perform auto-vectorization on loops containing prefetch instructions. 
            It must fall back to generating a scalar loop (processing only one 
            integer at a time).

        -   Instruction-level parallelism is reduced: SIMD instructions process 
            4/8/16 data elements at once, while scalar instructions process only 
            1 element at a time. This immediately results in a potential 4-16x 
            performance loss, which far outweighs any benefits that prefetching 
            might bring.
*/
__attribute__((optimize("no-tree-vectorize")))
void NoPrefetch(benchmark::State& state) 
{
    // Create a large vector to iterate over
    std::vector<int> data(state.range(0), 1);
    for (auto _ : state) 
    {
        long sum = 0;
        for (const auto& i : data) 
        {
            sum += i;
        }
        // Prevent compiler optimization to discard the sum
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(NoPrefetch)->Arg(1<<20); // Run with 1MB of data (2^20 integers)


// Function with __builtin_prefetch
void WithPrefetch(benchmark::State& state) {
    // Create a large vector to iterate over
    std::vector<int> data(state.range(0), 1);
    for (auto _ : state) 
    {
        long sum = 0;
        int prefetch_distance = 10;

        for (size_t i = 0; i < data.size(); i++) 
        {
            if (i + prefetch_distance < data.size()) 
            {
    	        __builtin_prefetch(&data[i + prefetch_distance], 0, 3);
            }
            sum += data[i];
        }
        
        // Prevent compiler optimization to discard the sum
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(WithPrefetch)->Arg(1<<20); // Run with 1MB of data (2^20 integers)

// Define matrix dimensions: 4000 * 4000
// This size is large enough to exceed typical CPU L3 cache capacities, 
// ensuring measurable differences in Cache Misses.
constexpr size_t ROWS = 4000;
constexpr size_t COLS = 4000;

// Global contiguous memory matrix
static std::vector<int>& GetGlobalMatrix() {
    static std::vector<int> matrix(ROWS * COLS);
    // Fill the matrix with sequential values
    static bool initialized = false;
    if (!initialized) {
        std::iota(matrix.begin(), matrix.end(), 0);
        initialized = true;
    }
    return matrix;
}

// 1. Hardware Prefetcher Friendly: Row-Major Traversal 
// (Sequential memory access, matches Simple Access Pattern)
static void BM_Matrix_Row_Major(benchmark::State& state) {
    auto& matrix = GetGlobalMatrix();
    
    for (auto _ : state) {
        long long sum = 0;
        // Outer loop: Rows
        for (size_t i = 0; i < ROWS; ++i) {
            // Inner loop: Columns (Memory addresses increment sequentially by 4 bytes)
            for (size_t j = 0; j < COLS; ++j) {
                sum += matrix[i * COLS + j];
            }
        }
        // Prevent the compiler from optimizing away the sum calculation as dead code
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Matrix_Row_Major);

// 2. Hardware Prefetcher Hostile: Column-Major Traversal 
// (Strided memory access, breaks spatial locality)
static void BM_Matrix_Col_Major(benchmark::State& state) {
    auto& matrix = GetGlobalMatrix();
    
    for (auto _ : state) {
        long long sum = 0;
        // Outer loop: Columns
        for (size_t j = 0; j < COLS; ++j) {
            // Inner loop: Rows (Each i++ leaps by 4000 * 4 = 16000 bytes, rendering the prefetcher useless)
            for (size_t i = 0; i < ROWS; ++i) {
                sum += matrix[i * COLS + j];
            }
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Matrix_Col_Major);

BENCHMARK_MAIN();
