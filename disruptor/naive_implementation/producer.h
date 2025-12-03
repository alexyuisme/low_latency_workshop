#include "queue.h"
#include <thread>

class Producer {
private:
    Queue& queue_;
public:
    Producer(Queue& queue) : queue_(queue) {}

    void produce(long data) {
        while (!queue_.add(data)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // slow down the producer a bit
        }
    }
};