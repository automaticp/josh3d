#pragma once
#include "Coroutines.hpp"
#include "ECS.hpp"
#include "DefaultResources.hpp"
#include "ResourceUnpacker.hpp"
#include "UUID.hpp"


namespace josh {


auto unpack_mesh(
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


/*
Convenience to automatically register all unpackers listed in this file. Optional.
*/
inline void register_default_unpackers(
    ResourceUnpacker& u)
{
    u.register_unpacker<RT::Mesh,     Handle>(&unpack_mesh);
    u.register_unpacker<RT::Material, Handle>(&unpack_material);
    u.register_unpacker<RT::MeshDesc, Handle>(&unpack_mdesc);
    u.register_unpacker<RT::Scene,    Handle>(&unpack_scene);
}


} // namespace josh
