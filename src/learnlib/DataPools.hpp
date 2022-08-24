#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "TextureData.hpp"



namespace learn {


class TextureDataPool {
private:
    using pool_t = std::unordered_map<std::string, std::shared_ptr<TextureData>>;
    pool_t pool_;

public:
    std::shared_ptr<TextureData> load(const std::string& path) {
        auto it = pool_.find(path);

        if ( it != pool_.end() ) {
            return it->second;
        } else {
            auto [emplaced_it, was_emplaced] = pool_.emplace(path, load_data_from(path));
            return emplaced_it->second;
        }
    }

private:
    std::shared_ptr<TextureData> load_data_from(const std::string& path) {
        return std::make_shared<TextureData>(StbImageData(path));
    }
};

// FIXME: This is destroyed after main() scope is over and OpenGL context is destroyed.
inline TextureDataPool global_texture_data_pool{};


} // namespace learn
