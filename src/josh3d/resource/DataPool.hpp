#pragma once
#include "Filesystem.hpp"
#include "TextureData.hpp"
#include "Shared.hpp"
#include <memory>
#include <string>
#include <unordered_map>


namespace josh {



template<typename T>
class DataPool {
private:
    using pool_t = std::unordered_map<File, Shared<T>>;
    pool_t pool_;

public:
    Shared<T> load(const File& file);

    void clear();
    void clear_unused();

private:
    // To be specialized externally
    Shared<T> load_data_from(const File& file);

};


template<typename T>
Shared<T> DataPool<T>::load(const File& file) {
    auto it = pool_.find(file);

    if ( it != pool_.end() ) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] =
            pool_.emplace(file, load_data_from(file));
        return emplaced_it->second;
    }
}

template<typename T>
void DataPool<T>::clear() {
    pool_.clear();
}

template<typename T>
void DataPool<T>::clear_unused() {
    for ( auto it = pool_.begin(); it != pool_.end(); ) {
        if ( it->second && it->second.use_count() == 1u ) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}


template<>
inline Shared<TextureData> DataPool<TextureData>::load_data_from(const File& file) {
    return std::make_shared<TextureData>(TextureData::from_file(file));
}


namespace globals {
inline DataPool<TextureData> texture_data_pool;
} // namespace globals


} // namespace josh
