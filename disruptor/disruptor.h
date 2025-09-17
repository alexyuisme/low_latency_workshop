#pragma once

#include "sequencer.h"
#include "event_processor.h"
#include "producer.h"
#include "wait_strategy.h"
#include <vector>
#include <thread>

template <size_t N, typename WaitStrategyDerived>
class Disruptor
{
public:
    explicit Disruptor(std::vector<EventProcessor<N>*>& processors, std::vector<Producer<N>*>& producers)
        : sequencer_(std::make_shared<Sequencer>()), processors_(processors), 
        producers_(producers), buffer_(std::make_shared<RingBuffer<N>>())
    {
        for (EventProcessor<N>* processor : processors_)
        {
            processor->set_sequencer(sequencer_);
        }

        for (Producer<N>* producer : producers_) {
            producer->set_sequencer(sequencer_);
        }
    }

    ~Disruptor() {}

    void start()
    {
        for (EventProcessor<N>* processor : processors_) {
            threads_.emplace_back([processor]() { processor->run(); });
        }
    }

    void halt()
    {
        // 1. 通知所有处理器停止
        for (EventProcessor<N>* processor : processors_) {
            processor->halt();
        }

        // 2. 等待所有线程正常结束
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }

        // 3. 清空线程数组
        threads_.clear();
    }

    long cursor() const
    {
        return sequencer_->cursor();
    }

private:
    std::shared_ptr<Sequencer> sequencer_;  // Changed from Sequencer to std::shared_ptr<Sequencer>
    std::vector<EventProcessor<N>*> processors_;
    std::vector<Producer<N>*> producers_;
    WaitStrategy<WaitStrategyDerived>* wait_strategy_;
    std::shared_ptr<RingBuffer<N>> buffer_;
    std::vector<std::thread> threads_;
};
