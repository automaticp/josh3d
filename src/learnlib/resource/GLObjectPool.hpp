#pragma once
#include "Shared.hpp"
#include "GLObjects.hpp"
#include "DataPool.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace learn {




template<typename T> struct default_upstream;

template<typename T>
using default_upstream_t = typename default_upstream<T>::type;



template<> struct default_upstream<TextureHandle> { using type = DataPool<TextureData>; };




template<typename T, typename UpstreamT = default_upstream_t<T>>
class GLObjectPool {
private:
    using pool_t = std::unordered_map<std::string, Shared<T>>;
    pool_t pool_;

    using upstream_t = UpstreamT;
    upstream_t& upstream_;

public:
    explicit GLObjectPool(UpstreamT& upstream) : upstream_{ upstream } {}

    Shared<T> load(const std::string& path);

    void clear();
    void clear_unused();

private:
    Shared<T> load_data_from(const std::string& path);

};


template<typename T, typename UpstreamT>
Shared<T> GLObjectPool<T, UpstreamT>::load(const std::string& path) {
    auto it = pool_.find(path);

    if ( it != pool_.end() ) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] =
            pool_.emplace(path, load_data_from(path));
        return emplaced_it->second;
    }
}

template<typename T, typename UpstreamT>
void GLObjectPool<T, UpstreamT>::clear() {
    pool_.clear();
}

template<typename T, typename UpstreamT>
void GLObjectPool<T, UpstreamT>::clear_unused() {
    for ( auto it = pool_.begin(); it != pool_.end(); ) {
        if ( it->second && it->second.use_count() == 1u ) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}








} // namespace learn
