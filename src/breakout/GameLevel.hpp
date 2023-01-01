#pragma once
#include "All.hpp"
#include "Matrix2D.hpp"
#include "Tile.hpp"
#include "Transform.hpp"
#include "Canvas.hpp"

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

public:
    GameLevel(const std::string& path)
        : tilemap_{ tilemap_from_file(path) }
    {
        build_level_from_tiles();
    }

    GameLevel(Matrix2D<TileType> tilemap)
        : tilemap_{ std::move(tilemap) }
    {
        build_level_from_tiles();
    }

    const std::vector<Tile>& tiles() const noexcept { return tiles_; }

private:



    void build_level_from_tiles() {
        auto [tile_width, tile_height]{ fit_tiles_to_grid() };
        const glm::vec3 tile_scale{ tile_width, tile_height, 1.f };

        for (size_t i{ 0 }; i < tilemap_.nrows(); ++i) {
            for (size_t j{ 0 }; j < tilemap_.ncols(); ++j) {

                TileType current_type{ tilemap_.at(i, j) };
                glm::vec3 current_transform{
                    (tile_width * static_cast<float>(j)),
                    (tile_height * static_cast<float>(i)),
                    1.0f
                };

                switch (current_type) {
                    case TileType::solid:
                    case TileType::brick_blue:
                    case TileType::brick_green:
                    case TileType::brick_red:
                    case TileType::brick_gold:
                        tiles_.emplace_back(
                            current_type,
                            learn::Transform()
                                .translate(current_transform)
                                .scale(tile_scale)
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

    struct TileGeometry { float width, height; };
    TileGeometry fit_tiles_to_grid() {
        return {
            .width = global_canvas.width() / static_cast<float>(tilemap_.ncols()),
            .height = 0.5f * global_canvas.height() / static_cast<float>(tilemap_.nrows())
        };
    }


public:
    // 2 beers later
    static Matrix2D<TileType> tilemap_from_file(const std::string& path) {
        auto text = learn::FileReader{}(path);
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
