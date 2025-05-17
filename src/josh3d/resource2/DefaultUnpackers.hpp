#pragma once
#include "Coroutines.hpp"
#include "ResourceUnpacker.hpp"
#include "ECS.hpp"
#include "UUID.hpp"


namespace josh {


class ResourceUnpacker;
class ResourceUnpackerContext;


/*
Convenience to automatically register all unpackers listed in this file. Optional.
*/
void register_default_unpackers(ResourceUnpacker& u);


auto unpack_static_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_skinned_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_material(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_mdesc(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_scene(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


} // namespace josh
