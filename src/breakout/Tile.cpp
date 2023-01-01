#include "Tile.hpp"


glm::vec3 Tile::get_color_for_type(TileType type) {
    using enum TileType;
    switch ( type ) {
        // Colors are temporary for now
        case solid: return { 1.f, 1.f, 1.f };
        case brick_blue: return { 0.2f, 0.6f, 1.0f };
        case brick_green: return { 0.0f, 0.7f, 0.0f };
        case brick_red: return { 1.0f, 0.5f, 0.0f };
        case brick_gold: return { 0.8f, 0.8f, 0.4f };
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
