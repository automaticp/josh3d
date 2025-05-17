#pragma once
#include "Coroutines.hpp"
#include "UUID.hpp"


namespace josh {


class ResourceLoader;
class ResourceLoaderContext;


void register_default_loaders(ResourceLoader& l);


auto load_static_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;


auto load_skinned_mesh(
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


} // namespace josh
