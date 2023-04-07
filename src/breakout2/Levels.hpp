#pragma once
#include "Matrix2D.hpp"
#include "PhysicsSystem.hpp"
#include "Tile.hpp"

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <string>
#include <cstddef>
#include <cassert>
#include <utility>
#include <vector>





class GameLevel {
private:
    Matrix2D<TileType> tilemap_;

    size_t max_num_alive_{ 0 };
    size_t num_alive_{ 0 };

public:
    GameLevel(Matrix2D<TileType> tilemap)
        : tilemap_{ std::move(tilemap) }
        , max_num_alive_{ count_breakable_tiles() }
    {}

    static GameLevel from_file(const std::string& path) {
        return GameLevel(tilemap_from_file(path));
    }

    void build_level_entities(entt::registry& registry, PhysicsSystem& physics);

    // Not a huge fan of these two below.
    // Maybe count the number of tiles separately?
    // Not very SRP otherwise...
    void report_destroyed_tile() noexcept {
        if (num_alive_ != 0) {
            --num_alive_;
        }
    }
    bool is_level_clear() const noexcept { return num_alive_ == 0; }




private:
    glm::vec2 scale_tiles_to_grid() const noexcept;
    size_t count_breakable_tiles() const noexcept;

    static glm::vec4 tile_color(TileType type);
    static std::string tile_texture_path(TileType type);

    static Matrix2D<TileType> tilemap_from_file(const std::string& path);
    static Matrix2D<TileType> tilemap_from_text(const std::string& text);
};



class Levels {
private:
    std::vector<GameLevel> levels_;
    size_t current_level_{ 0 };

public:
    void emplace(GameLevel lvl) {
        levels_.emplace_back(std::move(lvl));
    }

    GameLevel& current() noexcept {
        assert(!levels_.empty());
        return levels_[current_level_];
    }

    const GameLevel& current() const noexcept {
        assert(!levels_.empty());
        return levels_[current_level_];
    }

};
