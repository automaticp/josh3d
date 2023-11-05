#pragma once
#include "HeightmapData.hpp"
#include "GLObjects.hpp"
#include "Mesh.hpp"


namespace josh::components {


struct TerrainChunk {
    HeightmapData hdata;
    Mesh mesh;
    UniqueTexture2D heightmap;
};


} // namespace josh::components
