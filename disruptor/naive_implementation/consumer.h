#include <iostream>
#include <thread>

#include "queue.h"

class Consumer {
private:
    Queue& queue_;
public:
    Consumer(Queue& queue) : queue_(queue){}

    void consume() {
        while (true) {
            long data;
            auto ret = queue_.poll(data);
            if (ret == false) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // slow down the producer a bit
            } else {
                
            }
        }
    }
};