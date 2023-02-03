#pragma once
#include <algorithm>
#include <queue>
#include <utility>


template<typename T>
class EventQueue {
private:
    std::queue<T> queue_;

public:
    constexpr void push(T event) {
        queue_.emplace(std::move(event));
    }

    constexpr T pop() {
        T value{ std::move(queue_.front()) };
        queue_.pop();
        return value;
    }

    constexpr bool empty() const noexcept {
        return queue_.empty();
    }

};


enum class InputEvent {
    lmove, lstop, rmove, rstop, launch_ball, exit
};

