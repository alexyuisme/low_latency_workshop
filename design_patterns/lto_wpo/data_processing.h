#pragma once

#include "math_operations.h"
#include <iostream>

class DataProcessor {
public:
    DataProcessor(int data_size) : size_(data_size), result_(0.0) {
        buffer_ = new double[data_size];
    }

    ~DataProcessor()
    {
            delete[] buffer_;

    }
    
    // 性能关键函数
    void process_data(const double* input)
    {
        // 复制数据到缓冲区
        for (int i = 0; i < size_; ++i) {
            buffer_[i] = input[i];
        }
        
        // 应用变换
        apply_transform(buffer_, size_);
        
        // 计算幅度 - 跨模块调用！
        result_ = calculate_vector_magnitude(buffer_, size_);
    }

    double get_result() const 
    { 
        return result_; 
    }
    
private:
    double* buffer_;
    int size_;
    double result_;
    
    // 私有辅助函数 - 使用static便于优化
    static void apply_transform(double* data, int size)
    {
        // static函数 - 编译器可以积极优化
        for (int i = 0; i < size; ++i) 
        {
            data[i] = std::sin(data[i]) + std::cos(data[i]);  // 数学运算
        }
    }
};
