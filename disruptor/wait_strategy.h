#pragma once

template <typename WaitStrategyDerived>
class WaitStrategy
{
public:
    __attribute__((always_inline))
    void wait(){
        static_cast<WaitStrategyDerived>(this)->waitImpl();
    }
};

