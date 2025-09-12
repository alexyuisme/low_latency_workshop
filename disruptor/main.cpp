#include "disruptor.h"
#include "ring_buffer.h"
#include "sequencer.h"
#include "event_processor.h"
#include "producer.h"
#include "yield_wait_strategy.h"

#include <vector>

int main()
{
    // create shared instances of the RingBuffer and Sequencer
    const size_t N = 1024;

    auto ring_buffer = std::make_shared<RingBuffer<N>>();
    auto sequencer = std::make_shared<Sequencer>();
    
    // Create the producer and consumer
    Producer<N> producer(ring_buffer, sequencer);
    EventProcessor<N> consumer(ring_buffer, sequencer, 0);

    std::vector<EventProcessor<N>*> processors = {&consumer};
    std::vector<Producer<N>*> producers = {&producer};

    // Create the Disruptor
    Disruptor<N, YieldWaitStrategy> disruptor(processors, producers);

    // Start the Disruptor
    disruptor.start();

    // Generate some data in the producer
    for (int i = 0; i < 100; ++i)
    {
        producer.on_data("Event " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // slow down the producer a bit
    }

    // Stop the Disruptor
    disruptor.halt();

    return 0;
}
