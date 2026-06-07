#include <benchmark/benchmark.h>
#include <vector>
#include <random>

const size_t ARRAY_SIZE = 100'000;

std::vector<int> generate_random_data() {
    std::vector<int> data(ARRAY_SIZE);
    std::mt19937 prng(42); // 固定種子確保可重複性
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& val : data) val = dist(prng);
    return data;
}

const auto raw_data = generate_random_data();

static void BM_UnsortedArraySum(benchmark::State& state) {
    auto data = raw_data; // 複製一份未排序的數據

    for (auto _ : state) {
        long long sum = 0;
        for (size_t i = 0; i < ARRAY_SIZE; ++i) {
            // 隨機數據導致這裡的分支預測瘋狂失敗，頻繁觸發 Pipeline Flush
            if (data[i] >= 128) {
                sum += data[i];
            }
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_UnsortedArraySum);

static void BM_SortedArraySum(benchmark::State& state) {
    auto data = raw_data;
    std::sort(data.begin(), data.end()); // 在基準測試開始前先排好序

    for (auto _ : state) {
        long long sum = 0;
        for (size_t i = 0; i < ARRAY_SIZE; ++i) {
            // 由於數據是有序的，分支預測器很快就能抓到規律，做到 100% 預測正確
            if (data[i] >= 128) {
                sum += data[i];
            }
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_SortedArraySum);

BENCHMARK_MAIN();