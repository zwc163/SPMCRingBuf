#include <atomic>
#include <thread>
#include <iostream>
// 单生产者多消费者无锁ring buf
template<class T>
class SPMCRingBuf
{
private:
    std::atomic<uint64_t> head_;
    std::atomic<uint64_t> tail_;
    std::atomic<uint64_t> len_; 
    std::atomic<uint64_t> cap_; 
    T* data_;
public:
    SPMCRingBuf(uint64_t cap){
        head_ = 0;
        tail_=0;
        len_ = 0;
        cap_=cap;
        data_ = (T*)malloc(sizeof(T) * cap);
    }
    ~SPMCRingBuf(){
        delete[] data_;
    }
    void push(const T& t) {
        while(len_.load() == cap_.load());
        uint64_t old_head = head_.load(std::memory_order_acquire);
        uint64_t new_head = (old_head +1) % cap_;
        while(!head_.compare_exchange_weak(old_head, new_head, std::memory_order_relaxed)){
            old_head = head_.load(std::memory_order_acquire);
            new_head = (old_head +1) % cap_;
        }
        memcpy(&data_[old_head], &t, sizeof(T));
        len_.fetch_add(1, std::memory_order_release);
    }
    bool pop(T* t){
        if(len_.load() == 0) {
            return false;
        }
        uint64_t old_tail = tail_.load(std::memory_order_acquire);
        uint64_t new_tail = (old_tail +1) % cap_;
        while(!tail_.compare_exchange_weak(old_tail, new_tail, std::memory_order_relaxed)){
            old_tail = tail_.load(std::memory_order_acquire);
            new_tail = (old_tail +1) % cap_;
        }
        memcpy(t, &data_[old_tail], sizeof(T));
        len_.fetch_sub(1, std::memory_order_release);
        return true;
    }
};
