#include <iostream>
#include <sys/time.h>  // 用于gettimeofday()
#include <x86intrin.h> // 用于__rdtsc()指令
#include <unistd.h>    // 用于usleep()
#include <benchmark/benchmark.h> // Google Benchmark框架
#include "disruptor.h"

// rdtsc() 函数
/*
    -   使用__rdtsc()内建函数读取时间戳计数器(TSC)
    -   返回自CPU启动以来的时钟周期数
    -   提供高精度的计时
*/
uint64_t rdtsc() { return __rdtsc(); }

/*
    tsc_in_milli = 1000.0 * (平均结束周期 - 平均开始周期) / 实际时间差(微秒)

    其中tsc指的是Time Stamp Counter

    这行代码是计算每毫秒的时钟周期数(cycles per millisecond) 的核心
    公式。让我详细分解：

    -   公式结构

        tsc_in_milli = 1000.0 * (平均结束周期 - 平均开始周期) / 实际时间差(微秒)

    -   详细分解:

        1.  分子部分：((ccend1 + ccend0) / 2 - (ccstart1 + ccstart0) / 2)

            平均结束周期 = (ccend1 + ccend0) / 2
            平均开始周期 = (ccstart1 + ccstart0) / 2
            周期差 = 平均结束周期 - 平均开始周期

            -   为什么要取平均值?

                -   ccstart0: 在gettimeofday()之前的周期数
                -   ccstart1: 在gettimeofday()之后的周期数
                -   取平均值是为了减少gettimeofday()函数调用本身的开销影响
        
        2.  ((todend.tv_sec - todstart.tv_sec) * 1000000UL + todend.tv_usec - todstart.tv_usec)

            时间差(微秒) = (结束秒数 - 开始秒数) * 1,000,000 + (结束微秒数 - 开始微秒数)

        3.  乘以1000.0:

            每微秒周期数 = 总周期数 / 总时间(微秒)
            每毫秒周期数 = 每微秒周期数 × 1000 = (总周期数 / 总时间微秒) × 1000            

            -   因为分母是微妙, 乘以1000转换为周期数/毫秒
            -   最终得到：周期数/毫秒
*/
inline uint64_t tsc_per_milli(bool force = false)
{
    static uint64_t tsc_in_milli{0}; // 静态变量，只计算一次

    if (tsc_in_milli && !force) return tsc_in_milli;

    // 测量开始和结束的时间戳
    uint64_t ccstart0, ccstart1, ccend0, ccend1;
    timeval  todstart{}, todend{};

    // 获取时间戳和系统时间的配对测量
    ccstart0 = rdtsc();
    gettimeofday(&todstart, nullptr);
    ccstart1 = rdtsc();
    usleep(10000);  // sleep for 10 milli seconds or 10000 micro seconds
    ccend0 = rdtsc();
    gettimeofday(&todend, nullptr);
    ccend1 = rdtsc();

    // 计算平均时钟周期和实际时间差
    tsc_in_milli = 1000.0 * ((ccend1 + ccend0) / 2 - (ccstart1 + ccstart0) / 2) /
                   ((todend.tv_sec - todstart.tv_sec) * 1000000UL + todend.tv_usec - todstart.tv_usec);
    return tsc_in_milli;
}

// tsc_to_nano() 函数
/*

-   将时钟周期差转换为纳秒时间
-   使用之前计算的每毫秒时钟周期数进行转换

*/

inline double tsc_to_nano(uint64_t tsc_diff) 
{ 
    return (1.0 * tsc_diff / tsc_per_milli()) * 1'000'000;
}

template<typename Queue>
uint64_t bm_




