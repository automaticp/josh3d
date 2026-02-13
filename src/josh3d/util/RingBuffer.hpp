#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Scalars.hpp"
#include <cassert>


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
class BadRingBuffer
{
public:
    auto size()     const noexcept -> usize { return size_; }
    bool is_empty() const noexcept { return size_ == 0; }

    auto back()       noexcept ->       T& { assert(not is_empty()); return data_[tail_]; }
    auto back() const noexcept -> const T& { assert(not is_empty()); return data_[tail_]; }

    void emplace_front(auto&&... args);
    auto pop_back() noexcept -> T;

private:
    Vector<T> data_;
    usize     capacity_ = 0; // Why is this even separate from vector capacity?
    usize     head_     = 0;
    usize     tail_     = 0;
    usize     size_     = 0;

    void append_one(auto&&... args);
    void grow_and_append_one(auto&&... args);
};




template<typename T>
void BadRingBuffer<T>::emplace_front(auto&&... args)
{
    if (size_ == capacity_)
        grow_and_append_one(FORWARD(args)...);
    else
        append_one(FORWARD(args)...);
}

template<typename T>
void BadRingBuffer<T>::append_one(auto&&... args)
{
    data_[head_] = T{ FORWARD(args)... };
    head_ = (head_ + 1) % capacity_;
    ++size_;
}

template<typename T>
void BadRingBuffer<T>::grow_and_append_one(auto&&... args)
{
    const auto new_capacity = capacity_ + 1;
    const auto new_size     = size_ + 1;
    Vector<T> new_storage;
    new_storage.reserve(new_capacity);

    for (usize i = 0; i < size_; ++i)
        new_storage.emplace_back(MOVE(data_[(tail_ + i) % capacity_]));

    new_storage.emplace_back(FORWARD(args)...);

    data_     = MOVE(new_storage);
    size_     = new_size;
    capacity_ = new_capacity;
    head_     = 0;
    tail_     = 0;
}

template<typename T>
auto BadRingBuffer<T>::pop_back() noexcept -> T
{
    assert(!is_empty());
    auto old_tail = tail_;
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    return MOVE(data_[old_tail]);
}


} // namespace josh
