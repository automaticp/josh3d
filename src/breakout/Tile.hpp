#pragma once
#include "Sprite.hpp"
#include "All.hpp"
#include "Globals.hpp"
#include "Transform.hpp"

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
    learn::Transform tranform_;
    Sprite sprite_;


public:
    Tile(TileType type, const learn::Transform& transform)
        : type_{ type },
        tranform_{ transform },
        sprite_{
            learn::globals::texture_handle_pool.load(get_texture_path_for_type(type)),
            get_color_for_type(type)
        }
    {}


    learn::Transform& transform() noexcept { return tranform_; }
    const learn::Transform& transform() const noexcept { return tranform_; }

    const Sprite& sprite() const noexcept { return sprite_; }

private:
    static glm::vec3 get_color_for_type(TileType type);
    static std::string get_texture_path_for_type(TileType type);
};
