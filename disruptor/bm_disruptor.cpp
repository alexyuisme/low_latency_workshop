#include <iostream>
#include <sys/time.h>  // For gettimeofday()
#include <x86intrin.h> // For the __rdtsc() instruction
#include <unistd.h>    // For usleep()
#include <benchmark/benchmark.h> // Google Benchmark framework
#include "disruptor.h"

// rdtsc() function
/*
    -   Use the __rdtsc() built-in function to read the timestamp counter (TSC)
    -   Return the number of clock cycles since the CPU started
    -   Provide high-precision timing
*/
uint64_t rdtsc() { return __rdtsc(); }

/*
    tsc_in_milli = 1000.0 * (average end cycle - average start cycle) / actual elapsed time (microseconds)

    Here tsc means Time Stamp Counter

    This line is the core formula for computing the number of clock cycles per millisecond
    (cycles per millisecond). Let me break it down in detail:

    -   Formula structure

        tsc_in_milli = 1000.0 * (average end cycle - average start cycle) / actual elapsed time (microseconds)

    -   Detailed breakdown:

        1.  Numerator: ((ccend1 + ccend0) / 2 - (ccstart1 + ccstart0) / 2)

            Average end cycle = (ccend1 + ccend0) / 2
            Average start cycle = (ccstart1 + ccstart0) / 2
            Cycle difference = average end cycle - average start cycle

            -   Why take the average?

                -   ccstart0: cycle count before gettimeofday()
                -   ccstart1: cycle count after gettimeofday()
                -   Taking the average reduces the overhead impact of the gettimeofday() calls themselves

        2.  ((todend.tv_sec - todstart.tv_sec) * 1000000UL + todend.tv_usec - todstart.tv_usec)

            Time difference (microseconds) = (end seconds - start seconds) * 1,000,000 + (end microseconds - start microseconds)

        3.  Multiply by 1000.0:

            Cycles per microsecond = total cycles / total time (microseconds)
            Cycles per millisecond = cycles per microsecond × 1000 = (total cycles / total time in microseconds) × 1000

            -   Because the denominator is in microseconds, multiplying by 1000 converts it to cycles per millisecond
            -   Final result: cycles per millisecond
*/
inline uint64_t tsc_per_milli(bool force = false)
{
    static uint64_t tsc_in_milli{0}; // Static variable; computed only once

    if (tsc_in_milli && !force) return tsc_in_milli;

    // Measure the start and end timestamps
    uint64_t ccstart0, ccstart1, ccend0, ccend1;
    timeval  todstart{}, todend{};

    // Pair the timestamp measurement with the system time measurement
    ccstart0 = rdtsc();
    gettimeofday(&todstart, nullptr);
    ccstart1 = rdtsc();
    usleep(10000);  // sleep for 10 milliseconds or 10000 microseconds
    ccend0 = rdtsc();
    gettimeofday(&todend, nullptr);
    ccend1 = rdtsc();

    // Compute the average clock cycles and actual elapsed time
    tsc_in_milli = 1000.0 * ((ccend1 + ccend0) / 2 - (ccstart1 + ccstart0) / 2) /
                   ((todend.tv_sec - todstart.tv_sec) * 1000000UL + todend.tv_usec - todstart.tv_usec);
    return tsc_in_milli;
}

// tsc_to_nano() function
/*

-   Convert the clock-cycle difference into nanoseconds
-   Use the previously computed cycles-per-millisecond value to convert

*/

inline double tsc_to_nano(uint64_t tsc_diff)
{
    return (1.0 * tsc_diff / tsc_per_milli()) * 1'000'000;
}

template<typename Queue>
uint64_t bm_




