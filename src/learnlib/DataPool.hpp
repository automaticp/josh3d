#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "TextureData.hpp"



namespace learn {



template<typename T>
using Shared = std::shared_ptr<T>;


template<typename T>
class DataPool {
private:
    using pool_t = std::unordered_map<std::string, Shared<T>>;
    pool_t pool_;

public:
    Shared<T> load(const std::string& path);

    void clear();
    void clear_unused();

private:
    // To be specialized externally
    Shared<T> load_data_from(const std::string& path);

};


template<typename T>
Shared<T> DataPool<T>::load(const std::string& path) {
    auto it = pool_.find(path);

    if ( it != pool_.end() ) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] =
            pool_.emplace(path, load_data_from(path));
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
inline Shared<TextureData> DataPool<TextureData>::load_data_from(const std::string& path) {
    return std::make_shared<TextureData>(StbImageData(path));
}





} // namespace learn
