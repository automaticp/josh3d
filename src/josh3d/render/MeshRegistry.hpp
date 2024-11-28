#pragma once
#include "ContainerUtils.hpp"
#include "GLAttributeTraits.hpp"
#include "MeshStorage.hpp"
#include <any>
#include <typeindex>
#include <unordered_map>
#include <utility>






namespace josh {


/*
A collection of `MeshStorage` for various types of `VertexT`.
*/
class MeshRegistry {
public:
    template<specializes_attribute_traits VertexT>
    void emplace_storage_for();

    template<specializes_attribute_traits VertexT>
    bool has_storage_for() const noexcept;

    template<specializes_attribute_traits VertexT>
    auto storage_for() -> MeshStorage<VertexT>*;

    template<specializes_attribute_traits VertexT>
    auto storage_for() const -> const MeshStorage<VertexT>*;

    template<specializes_attribute_traits VertexT>
    auto ensure_storage_for() -> MeshStorage<VertexT>&;

    template<specializes_attribute_traits VertexT>
    bool remove_storage_for();

private:
    std::unordered_map<std::type_index, std::any> storages_;
};




template<specializes_attribute_traits VertexT>
void MeshRegistry::emplace_storage_for() {
    using storage_type = MeshStorage<VertexT>;
    storages_.emplace(std::type_index(typeid(VertexT)), std::in_place_type<storage_type>);
}


template<specializes_attribute_traits VertexT>
bool MeshRegistry::has_storage_for() const noexcept {
    return storages_.contains(std::type_index(typeid(VertexT)));
}


template<specializes_attribute_traits VertexT>
auto MeshRegistry::storage_for()
    -> MeshStorage<VertexT>*
{
    using storage_type = MeshStorage<VertexT>;
    if (auto* item = try_find(storages_, std::type_index(typeid(VertexT)))) {
        return std::any_cast<storage_type>(&item->second);
    } else {
        return nullptr;
    }
}


template<specializes_attribute_traits VertexT>
auto MeshRegistry::storage_for() const
    -> const MeshStorage<VertexT>*
{
    using storage_type = MeshStorage<VertexT>;
    if (const auto* item = try_find(storages_, std::type_index(typeid(VertexT)))) {
        return std::any_cast<storage_type>(&item->second);
    } else {
        return nullptr;
    }
}


template<specializes_attribute_traits VertexT>
auto MeshRegistry::ensure_storage_for()
    -> MeshStorage<VertexT>&
{
    using storage_type = MeshStorage<VertexT>;
    auto [it, was_emplaced] =
        storages_.try_emplace(std::type_index(typeid(VertexT)), std::in_place_type<storage_type>);
    return *std::any_cast<storage_type>(&it->second);
}


template<specializes_attribute_traits VertexT>
bool MeshRegistry::remove_storage_for() {
    return storages_.erase(std::type_index(typeid(VertexT)));
}





} // namespace josh
