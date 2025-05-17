#pragma once
#include "Coroutines.hpp"
#include "ResourceLoader.hpp"
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


auto load_material(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_texture(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_skeleton(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_animation(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_scene(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


inline void register_default_loaders(
    ResourceLoader& l)
{
    l.register_loader<RT::Mesh>     (&load_mesh);
    l.register_loader<RT::Texture>  (&load_texture);
    l.register_loader<RT::MeshDesc> (&load_mdesc);
    l.register_loader<RT::Material> (&load_material);
    l.register_loader<RT::Scene>    (&load_scene);
    l.register_loader<RT::Skeleton> (&load_skeleton);
    l.register_loader<RT::Animation>(&load_animation);
}


} // namespace josh
