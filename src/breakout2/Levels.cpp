#include "Levels.hpp"
#include "GlobalsGL.hpp"
#include "Transform2D.hpp"
#include "SpriteRenderSystem.hpp"
#include "Canvas.hpp"
#include <algorithm>
#include <range/v3/all.hpp>
#include <stdexcept>



void GameLevel::build_level_entities(entt::registry& registry, PhysicsSystem& physics) {

    const glm::vec2 tile_scale{ scale_tiles_to_grid() };

    for (size_t i{ 0 }; i < tilemap_.nrows(); ++i) {
        for (size_t j{ 0 }; j < tilemap_.ncols(); ++j) {

            TileType current_type{ tilemap_.at(i, j) };
            glm::vec2 current_center = glm::vec2{
                (tile_scale.x * static_cast<float>(j)) + tile_scale.x / 2.f,
                canvas.bound_top() - (tile_scale.y * static_cast<float>(i)) - tile_scale.y / 2.f
            };

            switch (current_type) {
                case TileType::solid:
                case TileType::brick_blue:
                case TileType::brick_green:
                case TileType::brick_red:
                case TileType::brick_gold: {
                    auto tile = registry.create();
                    registry.emplace<Transform2D>(tile, current_center, tile_scale, 0.f);
                    registry.emplace<Sprite>(tile,
                        learn::globals::texture_handle_pool.load(tile_texture_path(current_type)),
                        ZDepth::foreground, tile_color(current_type)
                    );
                    registry.emplace<PhysicsComponent>(tile,
                        physics.create_tile(tile, current_center, tile_scale)
                    );
                    registry.emplace<TileComponent>(tile, current_type);
                } break;
                case TileType::empty:
                    continue;
                default:
                    throw std::runtime_error("Invalid tile type. Cannot build level.");
            }
        }
    }

    num_alive_ = max_num_alive_;
}


glm::vec2 GameLevel::scale_tiles_to_grid() const noexcept {
    return {
        /* width  */ canvas.width() / static_cast<float>(tilemap_.ncols()),
        /* height */ 0.5f * canvas.height() / static_cast<float>(tilemap_.nrows())
    };
}


size_t GameLevel::count_breakable_tiles() const noexcept {
    return std::count_if(tilemap_.begin(), tilemap_.end(), [](TileType type) {
        using enum TileType;
        return type == brick_blue || type == brick_red
            || type == brick_green || type == brick_gold;
    });
}




glm::vec4 GameLevel::tile_color(TileType type) {
    switch (type) {
        using enum TileType;
        case solid: return { 1.f, 1.f, 1.f, 1.f };
        case brick_blue: return { 0.2f, 0.6f, 1.0f, 1.f };
        case brick_green: return { 0.0f, 0.7f, 0.0f, 1.f };
        case brick_red: return { 1.0f, 0.5f, 0.0f, 1.f };
        case brick_gold: return { 0.8f, 0.8f, 0.4f, 1.f };
        case empty:
        default:
            throw std::runtime_error("Invalid tile type. Does not have a color.");
    }
}


std::string GameLevel::tile_texture_path(TileType type) {
    switch (type) {
        using enum TileType;
        case solid: return "src/breakout2/sprites/block_solid.png";
        case brick_blue:
        case brick_green:
        case brick_gold:
        case brick_red: return "src/breakout2/sprites/block.png";
        case empty:
        default:
            throw std::runtime_error("Invalid tile type. Does not have a texture.");
    }
}




Matrix2D<TileType> GameLevel::tilemap_from_file(const std::string& path) {
    auto text = learn::read_file(path);
    return tilemap_from_text(text);
}


Matrix2D<TileType> GameLevel::tilemap_from_text(const std::string& text) {

    Matrix2D<TileType> tiles;

    auto rows = ranges::views::split(text, '\n');

    for (auto&& row : rows) {
        auto r = row | ranges::views::split(' ') |
            ranges::views::transform([](auto&& elem) {
                return TileType{ std::stoull(ranges::to<std::string>(elem)) };
            });
        tiles.push_row(r);
    }

    return tiles;
}
