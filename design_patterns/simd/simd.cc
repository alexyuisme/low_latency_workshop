#include <vector>
#include <iostream>
#include <emmintrin.h> // SSE2
#include <benchmark/benchmark.h>

// Note
/*
    With the compiler optimization flag -O3 enabled (e.g. -O3), 
    AddArrays() has the same speed AddArraysSIMD because of 
    auto-vectorization
*/

void GenerateTestData(float* a, float* b, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        a[i] = 1.0;
        b[i] = 2.0;
    }
}

void AddArraysSIMD(float* a, float* b, float* c, size_t size)
{
    __m128 a_chunk, b_chunk, c_chunk; // 声明3个128位向量寄存器

    for (size_t i = 0; i < size; i += 4) // 每次循环处理4个float
    {
        a_chunk = _mm_loadu_ps(&a[i]); // 从内存非对齐加载4个float到a_chunk
        b_chunk = _mm_loadu_ps(&b[i]); // 从内存非对齐加载4个float到b_chunk
        c_chunk = _mm_add_ps(a_chunk, b_chunk);  // SIMD加法：4个float同时相加
        _mm_storeu_ps(&c[i], c_chunk);   // 将结果非对齐存储回内存
    }
}

void AddArrays(float* a, float* b, float* c, size_t size)
{
	for (size_t i = 0; i < size; ++i) 
    {
    	c[i] = a[i] + b[i];
	}
}

static void ArrayAddition(benchmark::State& state)
{
    const size_t size = 10000; // 测试数组大小
    
    float* a = new float[size];
	float* b = new float[size];
	float* c = new float[size];

    GenerateTestData(a, b, size);

	for (auto _ : state) 
    {
        AddArrays(a, b, c, size);
	}

    benchmark::DoNotOptimize(c);

    delete[] a;
    delete[] b;
    delete[] c;
}
BENCHMARK(ArrayAddition);

static void ArrayAdditionSIMD(benchmark::State& state)
{
    const size_t size = 10000; // 测试数组大小
    
    float* a = new float[size];
	float* b = new float[size];
	float* c = new float[size];

    GenerateTestData(a, b, size);

	for (auto _ : state) 
    {
        AddArraysSIMD(a, b, c, size);
	}

    benchmark::DoNotOptimize(c);

    delete[] a;
    delete[] b;
    delete[] c;
}
BENCHMARK(ArrayAdditionSIMD);


BENCHMARK_MAIN();
