#include <vector>
#include <iostream>
#include <emmintrin.h> // SSE
#include <immintrin.h> // AVX
#include <benchmark/benchmark.h>

void GenerateTestData(float* a, float* b, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        a[i] = 1.0;
        b[i] = 2.0;
    }
}

// __attribute__((optimize("no-tree-vectorize")))
void AddArrays(float* a, float* b, float* c, size_t size)
{
	for (size_t i = 0; i < size; ++i) 
    {
    	c[i] = a[i] + b[i];
        benchmark::ClobberMemory(); // asm volatile ("" : : : "memory");
	}
}

static void BM_ArrayAddition(benchmark::State& state)
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
BENCHMARK(BM_ArrayAddition);

// SSE2
void AddArraysSSE(float* a, float* b, float* c, size_t size)
{
    __m128 a_chunk, b_chunk, c_chunk; // 声明3个128位向量寄存器

    for (size_t i = 0; i < size; i += 4) // 每次循环处理4个float
    {
        a_chunk = _mm_loadu_ps(&a[i]); // 从内存非对齐加载4个float到a_chunk
        b_chunk = _mm_loadu_ps(&b[i]); // 从内存非对齐加载4个float到b_chunk
        c_chunk = _mm_add_ps(a_chunk, b_chunk);  // SIMD加法：4个float同时相加
        _mm_storeu_ps(&c[i], c_chunk);   // 将结果非对齐存储回内存

        benchmark::ClobberMemory(); // asm volatile ("" : : : "memory");
    }
}

static void BM_AddArraysSSE(benchmark::State& state)
{
    const size_t size = 10000; // 测试数组大小
    
    float* a = new float[size];
	float* b = new float[size];
	float* c = new float[size];

    GenerateTestData(a, b, size);

	for (auto _ : state) 
    {
        AddArraysSSE(a, b, c, size);
	}

    benchmark::DoNotOptimize(c);

    delete[] a;
    delete[] b;
    delete[] c;
}
BENCHMARK(BM_AddArraysSSE);

// avx2
void AddArraysAVX2(float* a, float* b, float* c, size_t size)
{
    __m256 a_chunk, b_chunk, c_chunk; // 声明3个256位向量寄存器

    for (size_t i = 0; i < size; i += 8) // 每次循环处理8个float
    {
        a_chunk = _mm256_loadu_ps(a + i); // 从内存非对齐加载8个float到a_chunk
        b_chunk = _mm256_loadu_ps(b + i); // 从内存非对齐加载8个float到b_chunk
        c_chunk = _mm256_add_ps(a_chunk, b_chunk);  // SIMD加法：8个float同时相加
        _mm256_storeu_ps(c + i, c_chunk);   // 将结果非对齐存储回内存

        benchmark::ClobberMemory(); // asm volatile ("" : : : "memory");
    }
}

static void BM_AddArraysAVX2(benchmark::State& state)
{
    const size_t size = 10000; // 测试数组大小
    
    float* a = new float[size];
	float* b = new float[size];
	float* c = new float[size];

    GenerateTestData(a, b, size);

	for (auto _ : state) 
    {
        AddArraysAVX2(a, b, c, size);
	}

    benchmark::DoNotOptimize(c);

    delete[] a;
    delete[] b;
    delete[] c;
}
BENCHMARK(BM_AddArraysAVX2);

// 需要使用-mavx512编译选项, 请确保cpu支持avx512
// void AddArraysAVX512(float* a, float* b, float* c, size_t size)
// {
//     __m512 a_chunk, b_chunk, c_chunk; // 声明3个512位向量寄存器

//     for (size_t i = 0; i < size; i += 16) // 每次循环处理16个float
//     {
//         a_chunk = _mm512_loadu_ps(a + i); // 从内存非对齐加载16个float到a_chunk
//         b_chunk = _mm512_loadu_ps(b + i); // 从内存非对齐加载16个float到b_chunk
//         c_chunk = _mm512_add_ps(a_chunk, b_chunk);  // SIMD加法：16个float同时相加
//         _mm512_storeu_ps(c + i, c_chunk);   // 将结果非对齐存储回内存

//         benchmark::ClobberMemory(); // asm volatile ("" : : : "memory");
//     }
// }

// static void BM_AddArraysAVX512(benchmark::State& state)
// {
//     const size_t size = 10000; // 测试数组大小
//     const size_t alignment = 16; // 多少字节对齐
    
//     // float* a = new float[size];
//     // float* b = new float[size];
// 	// float* c = new float[size];
//     // std::cout << "max_align_t: " << alignof(std::max_align_t) << " bytes\n";
//     float* a = static_cast<float*>(aligned_alloc(alignment, size * sizeof(float)));

//     if (a == nullptr) {
//         throw std::bad_alloc();
//     }
//     float* b = static_cast<float*>(aligned_alloc(alignment, size * sizeof(float)));
//     float* c = static_cast<float*>(aligned_alloc(alignment, size * sizeof(float)));


//     GenerateTestData(a, b, size);

// 	for (auto _ : state) 
//     {
//         AddArraysAVX512(a, b, c, size);
// 	}

//     benchmark::DoNotOptimize(c);

//     delete[] a;
//     delete[] b;
//     delete[] c;
// }
// BENCHMARK(BM_AddArraysAVX512);

BENCHMARK_MAIN();