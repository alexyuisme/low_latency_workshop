#pragma once

#include "sequencer.h"
#include "ring_buffer.h"
#include <iostream>

template <size_t N>
class Producer
{
public:
    Producer(std::shared_ptr<RingBuffer<N>> ring_buffer, std::shared_ptr<Sequencer> sequencer)
        : ring_buffer_(ring_buffer), sequencer_(sequencer)
    {
        std::cout << "Producer Sequencer Addr: " << sequencer_.get() << std::endl;   
    }

    void on_data(const std::string& data)
    {
        long sequence = sequencer_->next();
        Event& event = ring_buffer_->get(sequence);
        event.set(data);
        sequencer_->publish(sequence);

        std::cout << "[Producer] Published: " << data << " at sequence: " << sequence << "\n";
    }

    void set_sequencer(std::shared_ptr<Sequencer> sequencer)
    {
        sequencer_ = sequencer;
        sequencer->cursor();
    }

private:
    std::shared_ptr<RingBuffer<N>> ring_buffer_;
    std::shared_ptr<Sequencer> sequencer_;
};
