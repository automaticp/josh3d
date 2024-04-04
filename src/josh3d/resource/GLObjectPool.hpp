#pragma once
#include "Filesystem.hpp"
#include "GLShared.hpp"
#include <unordered_map>


namespace josh {


template<
    typename T,
    typename UpstreamT,
    typename LoadContextT
>
class GLObjectPool {
private:
    using pool_t = std::unordered_map<File, GLShared<T>>;
    pool_t pool_;

    using upstream_t = UpstreamT;
    upstream_t& upstream_;

public:
    explicit GLObjectPool(UpstreamT& upstream) : upstream_{ upstream } {}

    GLShared<T> load(const File& file, const LoadContextT& = LoadContextT{});

    void clear() { pool_.clear(); }
    void clear_unused();

private:
    GLShared<T> load_data_from(const File& file, const LoadContextT&);

};




template<typename T, typename UpstreamT, typename LoadContextT>
auto GLObjectPool<T, UpstreamT, LoadContextT>::load(
    const File&         file,
    const LoadContextT& context)
        -> GLShared<T>
{
    auto it = pool_.find(file);

    if (it != pool_.end()) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] =
            pool_.emplace(file, load_data_from(file, context));

        return emplaced_it->second;
    }
}


template<typename T, typename UpstreamT, typename LoadContextT>
void GLObjectPool<T, UpstreamT, LoadContextT>::clear_unused() {

    for (auto it = pool_.begin();
        it != pool_.end();)
    {
        if (it->second && it->second.use_count() == 1u) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }

}




} // namespace josh
