#pragma once
#include "Sprite.hpp"
#include "All.hpp"
#include "Globals.hpp"
#include "Transform.hpp"
#include "Rect2D.hpp"

#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <stdexcept>
#include <unordered_map>

enum class TileType : size_t {
    empty = 0,
    solid = 1,
    brick_blue = 2,
    brick_green = 3,
    brick_gold = 4,
    brick_red = 5,
};



class Tile {
private:
    TileType type_;
    Rect2D box_;
    Sprite sprite_;

public:
    Tile(TileType type, Rect2D bounding_box)
        : type_{ type },
        box_{ bounding_box },
        sprite_{
            learn::globals::texture_handle_pool.load(get_texture_path_for_type(type)),
            get_color_for_type(type)
        }
    {}

    learn::Transform get_transform() const noexcept { return box_.get_transform(); }

    const Sprite& sprite() const noexcept { return sprite_; }

private:
    static glm::vec3 get_color_for_type(TileType type);
    static std::string get_texture_path_for_type(TileType type);
};
