#pragma once
#include "FXType.hpp"
#include <entt/entity/fwd.hpp>
#include <algorithm>
#include <queue>
#include <utility>

// A wrapper around a queue which is a wrapper around a deque...
// Less functions, less trouble?
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


struct TileCollisionEvent {
    entt::entity tile_entity;
};


enum class FXToggleType : bool {
    disable, enable
};

struct FXToggleEvent {
    FXType type;
    FXToggleType toggle_type;
};




// TF is this name though?
class EventBus {
private:
    EventQueue<InputEvent> input;
    EventQueue<TileCollisionEvent> tile_collision;
    EventQueue<FXToggleEvent> fx_toggle;

    friend class Game;

public:
    void push_input_event(InputEvent event) {
        input.push(event);
    }

    void push_tile_collision_event(TileCollisionEvent event) {
        tile_collision.push(event);
    }

    void push_fx_toggle_event(FXToggleEvent event) {
        fx_toggle.push(event);
    }
};

// Event queues are global and are exposed to all the classes.
// ANYONE can push an event, but it's the responsibility of
// the Game, namely Game::process_events(), to process them.
// This is enforced through the power of **friendship**.
inline EventBus events;
