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


auto unpack_material_diffuse(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_material_normal(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_material_specular(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;


auto unpack_mdesc(
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
    u.register_unpacker<RT::Mesh, Handle>(&unpack_mesh);
    // FIXME: Once again we have a problem since we do not describe materials consistently.
    u.register_unpacker<RT::MeshDesc, Handle>(&unpack_mdesc);
}


} // namespace josh
