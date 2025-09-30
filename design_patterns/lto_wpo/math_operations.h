#pragma once

#include <cmath>

// 没有static - 跨模块调用时可能无法内联
double calculate_vector_magnitude(const double* data, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        sum += data[i] * data[i];  // 热点循环
    }
    return std::sqrt(sum);
}

// 使用static - 限制在当前模块，便于优化
static double square(double x) {
    return x * x;  // 简单函数，内联收益大
}

double dot_product(const double* a, const double* b, int size) {
    double result = 0.0;
    for (int i = 0; i < size; ++i) {
        result += a[i] * b[i];  // 另一个热点循环
    }
    return result;
}

void normalize_vector(double* data, int size) {
    double mag = calculate_vector_magnitude(data, size);
    if (mag != 0.0) {
        for (int i = 0; i < size; ++i) {
            data[i] /= mag;
        }
    }
}

double calculate_distance(const double* a, const double* b, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        double diff = a[i] - b[i];
        sum += square(diff);  // 调用static函数，可以内联
    }
    return std::sqrt(sum);
}
