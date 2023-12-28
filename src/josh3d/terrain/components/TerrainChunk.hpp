#pragma once
#include "ImageData.hpp"
#include "Pixels.hpp"
#include "GLObjects.hpp"
#include "Mesh.hpp"


namespace josh::components {


struct TerrainChunk {
    ImageData<pixel::REDF> hdata;
    Mesh mesh;
    UniqueTexture2D heightmap;
};


} // namespace josh::components
