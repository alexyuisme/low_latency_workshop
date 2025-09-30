#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "data_processing.h"
#include "math_operations.h"

// 生成测试数据
std::vector<double> generate_test_data(int size) {
    std::vector<double> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    
    for (int i = 0; i < size; ++i) {
        data[i] = dist(gen);
    }
    return data;
}

// 性能测试函数
void benchmark_without_lto() {
    const int data_size = 100000;
    const int iterations = 1000;
    
    auto test_data = generate_test_data(data_size);
    DataProcessor processor(data_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        processor.process_data(test_data.data());
        // 防止编译器过度优化
        asm volatile("" : : "r,m"(processor.get_result()) : "memory");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Without LTO: " << duration.count() << " ms" << std::endl;
}

void benchmark_with_lto() {
    const int data_size = 100000;
    const int iterations = 1000;
    
    auto test_data = generate_test_data(data_size);
    DataProcessor processor(data_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        processor.process_data(test_data.data());
        asm volatile("" : : "r,m"(processor.get_result()) : "memory");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "With LTO: " << duration.count() << " ms" << std::endl;
}

int main() {
    std::cout << "Benchmarking LTO performance impact..." << std::endl;
    
    // 测试没有LTO的性能
    benchmark_without_lto();
    
    // 测试有LTO的性能  
    //benchmark_with_lto();
    
    // 演示跨模块内联
    std::vector<double> vec1 = {1.0, 2.0, 3.0};
    std::vector<double> vec2 = {4.0, 5.0, 6.0};
    
    double dot = dot_product(vec1.data(), vec2.data(), 3);
    double dist = calculate_distance(vec1.data(), vec2.data(), 3);
    
    std::cout << "Dot product: " << dot << std::endl;
    std::cout << "Distance: " << dist << std::endl;
    
    return 0;
}
