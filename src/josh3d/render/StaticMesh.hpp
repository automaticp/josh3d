#pragma once
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "Scalars.hpp"
#include "VertexStatic.hpp"
#include "LODPack.hpp"


namespace josh {

using StaticMeshID = MeshID<VertexStatic>;

struct StaticMesh
{
    LODPack<StaticMeshID, 8> lods;
    ResourceUsage            usage   = {}; // One usage for all MeshIDs.
    uintptr                  aba_tag = {};
};


} // namespace josh
