#pragma once
#include "Coroutines.hpp"
#include "ResourceRegistry.hpp"
#include "DefaultResources.hpp"
#include "UUID.hpp"


namespace josh {


auto load_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_mdesc(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_texture(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_scene(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


inline void register_default_loaders(
    ResourceRegistry& r)
{
    r.register_resource<RT::Mesh>(&load_mesh);
    r.register_resource<RT::Texture>(&load_texture);
    r.register_resource<RT::MeshDesc>(&load_mdesc);
    r.register_resource<RT::Scene>(&load_scene);
}

} // namespace josh
