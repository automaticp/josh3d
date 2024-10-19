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


auto create_terrain_chunk(
    float           max_height,
    const Extent2F& extents,
    const Size2S&   resolution)
        -> TerrainChunk;


} // namespace josh
