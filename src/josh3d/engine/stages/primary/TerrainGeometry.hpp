#pragma once
#include "StageContext.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"


namespace josh {


struct TerrainGeometry
{
    void operator()(PrimaryContext context);

    ShaderToken _sp = shader_pool().get({
        .vert = VPath("src/shaders/dfrg_terrain.vert"),
        .frag = VPath("src/shaders/dfrg_terrain.frag")});
};


} // namespace josh
