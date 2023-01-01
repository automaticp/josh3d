#include "Tile.hpp"


glm::vec3 Tile::get_color_for_type(TileType type) {
    using enum TileType;
    switch ( type ) {
        // Colors are temporary for now
        case solid: return { 1.f, 1.f, 1.f };
        case brick_blue: return { 0.f, 0.f, 1.f };
        case brick_green: return { 0.f, 1.f, 0.f };
        case brick_red: return { 1.f, 0.f, 0.f };
        case brick_gold: return { 1.f, 1.f, 0.f };
        case empty:
        default:
            throw std::runtime_error("Invalid tile type. Does not have a color.");
    }
}


std::string Tile::get_texture_path_for_type(TileType type) {
    switch ( type ) {
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
