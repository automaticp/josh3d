#pragma once
#include "All.hpp"
#include "FrameTimer.hpp"




enum class GameState {
    active, win, menu
};


class Game {
private:
    GameState state_;
    const learn::FrameTimer& frame_timer_;

public:
    Game(learn::FrameTimer& frame_timer)
        : frame_timer_( frame_timer ) {}

    Game()
        : frame_timer_( learn::global_frame_timer ) {}

    void init();

    void process_input();
    void update();
    void render();

};
