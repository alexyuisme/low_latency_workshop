#pragma once

#include "sequencer.h"
#include "ring_buffer.h"
#include <iostream>

template <size_t N>
class EventProcessor
{
public:
    EventProcessor(std::shared_ptr<RingBuffer<N>> ring_buffer, std::shared_ptr<Sequencer> sequencer, int id)
        : running_(true), next_sequence_(0), ring_buffer_(ring_buffer), sequencer_(sequencer), id_(id)
    {
        std::cout << "EventProcessor Sequencer Addr: " << sequencer_.get() << std::endl;
    }

    void run()
    {
        //std::cout << "Consumer running. Waiting for events...\n";
        while (running_)
        {
            //std::cout << "Consumer is in the while loop. Next sequence to read: " << nextSequence_ << "\n";
            while (next_sequence_ <= sequencer_->cursor())
            {
                Event& event = ring_buffer_->get(next_sequence_);
                std::cout << "[Consumer " << id_ << " ]" << " Consumed: " << event.get() << " from sequence: " << next_sequence_ << "\n";
                ++next_sequence_;
            }
        }
    }

    void stop()
    {

    }

    void halt()
    {
        running_ = false;
    }

    // If you want to be able to set the Sequencer dynamically
    void set_sequencer(std::shared_ptr<Sequencer> sequencer)
    {
        sequencer_ = sequencer;
    }

private:
    bool running_;
    long next_sequence_;
    std::shared_ptr<RingBuffer<N>> ring_buffer_;
    std::shared_ptr<Sequencer> sequencer_;
    int id_;
};