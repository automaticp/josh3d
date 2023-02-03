#pragma once
#include <cstddef>


enum class TileType : size_t {
    empty = 0,
    solid = 1,
    brick_blue = 2,
    brick_green = 3,
    brick_gold = 4,
    brick_red = 5,
};


struct TileComponent {
    TileType type;
};

