#include "disruptor.h"
#include "yield_wait_strategy.h"

#include <gtest/gtest.h>

/*  


g++ -std=c++20 -pthread -I. -I/usr/local/include \
    gtest_disruptor.cpp \
    -lgtest -lgtest_main -lpthread \
    -O3 \
    -o gtest_disruptor
*/

// single producer multiple consumer
TEST(DisruptorTest, SPMCTest)
{
        // create shared instances of the RingBuffer and Sequencer
    const size_t N = 1024;

    auto ring_buffer = std::make_shared<RingBuffer<N>>();
    auto sequencer = std::make_shared<Sequencer>();
    
    // Create the producer and consumer
    Producer<N> producer(ring_buffer, sequencer); // the sequencer will be replaced 
    EventProcessor<N> consumer1(ring_buffer, sequencer, 0); // the sequencer will be replaced
    EventProcessor<N> consumer2(ring_buffer, sequencer, 1); // the sequencer will be replaced

    std::vector<EventProcessor<N>*> processors = {&consumer1, &consumer2};
    std::vector<Producer<N>*> producers = {&producer};

    // Create the Disruptor
    Disruptor<N, YieldWaitStrategy> disruptor(processors, producers);

    // Start the Disruptor
    disruptor.start();

    EXPECT_EQ(disruptor.cursor(), -1);

    // produce one data
    producer.on_data("Event " + std::to_string(0));

    // Wait for consumers finishing all taskes.
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // slow down the producer a bit

    EXPECT_EQ(disruptor.cursor(), 0);

    producer.on_data("Event " + std::to_string(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // slow down the producer a bit
    EXPECT_EQ(disruptor.cursor(), 1);

    // Stop the Disruptor
    disruptor.halt();
}

TEST(DisruptorTest, BasicTest)
{
    // create shared instances of the RingBuffer and Sequencer
    const size_t N = 1024;

    auto ring_buffer = std::make_shared<RingBuffer<N>>();
    auto sequencer = std::make_shared<Sequencer>();
    
    // Create the producer and consumer
    Producer<N> producer(ring_buffer, sequencer); // the sequencer will be replaced 
    EventProcessor<N> consumer(ring_buffer, sequencer, 0); // the sequencer will be replaced

    std::vector<EventProcessor<N>*> processors = {&consumer};
    std::vector<Producer<N>*> producers = {&producer};

    // Create the Disruptor
    Disruptor<N, YieldWaitStrategy> disruptor(processors, producers);

    // Start the Disruptor
    disruptor.start();

    EXPECT_EQ(disruptor.cursor(), -1);

    // produce one data
    producer.on_data("Event " + std::to_string(0));

    // Wait for consumers finishing all taskes.
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // slow down the producer a bit

    EXPECT_EQ(disruptor.cursor(), 0);

    producer.on_data("Event " + std::to_string(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // slow down the producer a bit
    EXPECT_EQ(disruptor.cursor(), 1);

    // Stop the Disruptor
    disruptor.halt();
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}