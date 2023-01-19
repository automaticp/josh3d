#pragma once
#include "All.hpp"
#include "Matrix2D.hpp"
#include "Tile.hpp"
#include "Transform.hpp"
#include "Canvas.hpp"

#include <algorithm>
#include <glm/fwd.hpp>
#include <range/v3/all.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <tuple>
#include <stdexcept>





class GameLevel {
private:
    Matrix2D<TileType> tilemap_;
    std::vector<Tile> tiles_;

    size_t num_alive_;
    size_t max_num_alive_;

public:
    GameLevel(const std::string& path)
        : GameLevel{ tilemap_from_file(path) }
    {}

    GameLevel(Matrix2D<TileType> tilemap)
        : tilemap_{ std::move(tilemap) }
    {
        build_level_from_tiles();
        num_alive_ = max_num_alive_ =
            std::count_if(
                tiles_.begin(), tiles_.end(),
                [](const Tile& tile) {
                    return tile.type() != TileType::solid && tile.is_alive();
                }
            );
    }

    std::vector<Tile>& tiles() noexcept { return tiles_; }
    const std::vector<Tile>& tiles() const noexcept { return tiles_; }

    void report_destroyed_tile() {
        if (num_alive_ != 0) {
            --num_alive_;
        }
    }

    bool is_level_clear() {
        return num_alive_ == 0;
    }

private:
    void build_level_from_tiles() {
        const glm::vec2 tile_scale{ scale_tiles_to_grid() };

        for (size_t i{ 0 }; i < tilemap_.nrows(); ++i) {
            for (size_t j{ 0 }; j < tilemap_.ncols(); ++j) {

                TileType current_type{ tilemap_.at(i, j) };
                glm::vec2 current_center = glm::vec2{
                    (tile_scale.x * static_cast<float>(j)) + tile_scale.x / 2.f,
                    global_canvas.bound_top() - (tile_scale.y * static_cast<float>(i)) - tile_scale.y / 2.f
                };

                switch (current_type) {
                    case TileType::solid:
                    case TileType::brick_blue:
                    case TileType::brick_green:
                    case TileType::brick_red:
                    case TileType::brick_gold:
                        tiles_.emplace_back(
                            current_type,
                            Rect2D{ current_center, tile_scale }
                        );
                        break;
                    case TileType::empty:
                        continue;
                    default:
                        throw std::runtime_error("Invalid tile type. Cannot build level.");
                }
            }
        }

    }

    glm::vec2 scale_tiles_to_grid() {
        return {
            /* width  */ global_canvas.width() / static_cast<float>(tilemap_.ncols()),
            /* height */ 0.5f * global_canvas.height() / static_cast<float>(tilemap_.nrows())
        };
    }


public:
    // 2 beers later
    static Matrix2D<TileType> tilemap_from_file(const std::string& path) {
        auto text = learn::read_file(path);
        return tilemap_from_text(text);
    }

    static Matrix2D<TileType> tilemap_from_text(const std::string& text) {

        Matrix2D<TileType> tiles;

        auto rows = ranges::views::split(text, '\n');

        for (auto&& row : rows) {
            auto r = row
                | ranges::views::split(' ')
                | ranges::views::transform([](auto&& elem) {
                    return TileType{
                        std::stoull(
                            ranges::to<std::string>(elem)
                        )
                    };
                });
            tiles.push_row(r);
        }

        return tiles;
    }

};
