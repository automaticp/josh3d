#pragma once
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "VertexStatic.hpp"
#include "LODPack.hpp"
#include <cstdint>


namespace josh {


struct StaticMesh {
    LODPack<MeshID<VertexStatic>, 8> lods;
    ResourceUsage                    usage;   // One usage for all MeshIDs.
    uintptr_t                        aba_tag{};
};


} // namespace josh
