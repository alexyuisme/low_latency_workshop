#include <vector>

class Queue {
private:
    std::vector<long> data_;
    size_t capacity_{0};
    size_t head_{0};
    size_t tail_{0};

public:
    Queue(size_t size) : capacity_(size+1) {
        data_ = std::vector<long>(capacity_);
    }

    bool add(long elem) {
        // How about adding an elem in the queue with size == 1, impossible to 
        // add??
        if ((tail_ + 1) % capacity_ == head_) return false;

        data_[tail_] = elem;
        tail_ = (tail_ + 1) % capacity_;
        return true;
    }

    bool poll(long& elem) {
        if (head_ == tail_) return false;

        elem = data_[head_];
        head_ = (head_ + 1) % capacity_;
        return true;
    }
};