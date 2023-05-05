#pragma once
#include "Shared.hpp"
#include "GLObjects.hpp"
#include "DataPool.hpp"
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace learn {




template<
    typename T,
    typename UpstreamT,
    typename LoadContextT
>
class GLObjectPool {
private:
    using pool_t = std::unordered_map<std::string, Shared<T>>;
    pool_t pool_;

    using upstream_t = UpstreamT;
    upstream_t& upstream_;

public:
    explicit GLObjectPool(UpstreamT& upstream) : upstream_{ upstream } {}

    Shared<T> load(const std::string& path, const LoadContextT& = LoadContextT{});

    void clear() { pool_.clear(); }
    void clear_unused();

private:
    Shared<T> load_data_from(const std::string& path, const LoadContextT&);

};


template<typename T, typename UpstreamT, typename LoadContextT>
Shared<T> GLObjectPool<T, UpstreamT, LoadContextT>::load(const std::string& path,
    const LoadContextT& context)
{
    auto it = pool_.find(path);

    if ( it != pool_.end() ) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] =
            pool_.emplace(
                path,
                load_data_from(path, context)
            );
        return emplaced_it->second;
    }
}


template<typename T, typename UpstreamT, typename LoadContextT>
void GLObjectPool<T, UpstreamT, LoadContextT>::clear_unused() {
    for ( auto it = pool_.begin(); it != pool_.end(); ) {
        if ( it->second && it->second.use_count() == 1u ) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}








} // namespace learn
