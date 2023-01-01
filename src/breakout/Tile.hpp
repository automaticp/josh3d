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
    static glm::vec3 get_color_for_type(TileType type) {
        using enum TileType;
        switch (type) {
            case solid:
                return { 1.f, 1.f, 1.f };
            // Colors are temporary for now
            case brick_blue:
                return { 0.f, 0.f, 1.f };
            case brick_green:
                return { 0.f, 1.f, 0.f };
            case brick_red:
                return { 1.f, 0.f, 0.f };
            case brick_gold:
                return { 1.f, 1.f, 0.f };
            case empty:
            default:
                throw std::runtime_error("Invalid tile type. Does not have a color.");
        }
    }


    static std::string get_texture_path_for_type(TileType type) {
        switch (type) {
            case TileType::solid:
                return "src/breakout/sprites/block_solid.png";
            case TileType::brick_blue:
            case TileType::brick_green:
            case TileType::brick_gold:
            case TileType::brick_red:
                return "src/breakout/sprites/block.png";
            case TileType::empty:
            default:
                throw std::runtime_error("Invalid tile type. Does not have a texture.");
        }
    }
};
