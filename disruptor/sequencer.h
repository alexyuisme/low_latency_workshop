#pragma once

#include <atomic>
#include <iostream>

class Sequencer
{
public:
    Sequencer() : cursor_(-1){
    }
    
    long next()
    {
        return ++cursor_;
    }

    void publish(long sequence)
    {
        cursor_ = sequence;
    }
    
    long cursor() const
    {
        return cursor_.load();
    }
private:
    std::atomic<long> cursor_;
};
