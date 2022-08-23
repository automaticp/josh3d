#pragma once
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include "DataPools.hpp"
#include "GLObjects.hpp"


namespace learn {



class TextureHandlePool {
private:
    using pool_t = std::unordered_map<std::string, std::shared_ptr<TextureHandle>>;
    pool_t pool_;

    using upstream_t = TextureDataPool;
    upstream_t& upstream_;

public:
    explicit TextureHandlePool(upstream_t& upstream) : upstream_{ upstream } {}

    std::shared_ptr<TextureHandle> load(const std::string& path) {
        auto it = pool_.find(path);

        if ( it != pool_.end() ) {
            return it->second;
        } else {
            auto [emplaced_it, was_emplaced] = pool_.emplace(path, load_data_from(path));
            return emplaced_it->second;
        }
    }

private:
    std::shared_ptr<TextureHandle> load_data_from(const std::string& path) {
        std::shared_ptr<TextureData> tex_data{ upstream_.load(path) };

        auto new_handle = std::make_shared<TextureHandle>();
        new_handle->bind().attach_data(*tex_data);

        return new_handle;
    }
};


inline TextureHandlePool global_texture_handle_pool{ global_texture_data_pool };


} // namespace learn
