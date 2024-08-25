#pragma once
#include "Pixels.hpp"
#include "PixelData.hpp"
#include "GLObjects.hpp"
#include "Mesh.hpp"


namespace josh {


struct TerrainChunk {
    Mesh                   mesh;
    PixelData<pixel::RedF> heightmap_data;
    UniqueTexture2D        heightmap;
};


} // namespace josh::components
