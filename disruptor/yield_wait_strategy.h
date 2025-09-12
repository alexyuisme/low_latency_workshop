#pragma once

#include <thread>
#include "wait_strategy.h"

class YieldWaitStrategy : public WaitStrategy<YieldWaitStrategy>
{
public:
    void waitImpl()
    {
        std::this_thread::yield();
    }
};