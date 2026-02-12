#pragma once
#include <cassert>
#include <cstddef>
#include <vector>


namespace josh {


/*
Questionable implementation that has no amortized reallocation,
and each reallocation is O(n). Also you can't reserve capacity.

This is because we otherwise need to control the lifetimes of
objects, and keep amortized memory uninitialized.
I would need some time to write that.

"Moved-from" objects are the only source of amortized memory here.
The buffer does not shrink, thankfully.

It's OK if you buffer never grows too large and is stable in max
size on average, I guess. Wrote this in a rush, sorry :(
*/
template<typename T>
class BadRingBuffer {
private:
    std::vector<T> data_;
    size_t capacity_{ 0 };
    size_t head_    { 0 };
    size_t tail_    { 0 };
    size_t size_    { 0 };

public:
    size_t size()     const noexcept { return size_; }
    bool   is_empty() const noexcept { return size_ == 0; }

          T& back()       noexcept { assert(!is_empty()); return data_[tail_]; }
    const T& back() const noexcept { assert(!is_empty()); return data_[tail_]; }

    void emplace_front(auto&&... args);
    auto pop_back() noexcept -> T;


private:
    void append_one(auto&&... args);
    void grow_and_append_one(auto&&... args);

};




template<typename T>
void BadRingBuffer<T>::emplace_front(auto&&... args) {
    if (size_ == capacity_) {
        grow_and_append_one(std::forward<decltype(args)>(args)...);
    } else {
        append_one(std::forward<decltype(args)>(args)...);
    }
}


template<typename T>
void BadRingBuffer<T>::append_one(auto&&... args) {
    data_[head_] = T{ std::forward<decltype(args)>(args)... };
    head_ = (head_ + 1) % capacity_;
    ++size_;
}


template<typename T>
void BadRingBuffer<T>::grow_and_append_one(auto&&... args) {
    const size_t new_capacity = capacity_ + 1;
    const size_t new_size     = size_ + 1;
    std::vector<T> new_storage;
    new_storage.reserve(new_capacity);

    for (size_t i{ 0 }; i < size_; ++i) {
        new_storage.emplace_back(
            std::move(data_[(tail_ + i) % capacity_])
        );
    }
    new_storage.emplace_back(std::forward<decltype(args)>(args)...);

    data_     = std::move(new_storage);
    size_     = new_size;
    capacity_ = new_capacity;
    head_     = 0;
    tail_     = 0;
}


template<typename T>
auto BadRingBuffer<T>::pop_back() noexcept
    -> T
{
    assert(!is_empty());
    auto old_tail = tail_;
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    return std::move(data_[old_tail]);
}


} // namespace josh
