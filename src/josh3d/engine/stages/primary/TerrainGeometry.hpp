#pragma once
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"


namespace josh::stages::primary {


class TerrainGeometry {
public:
    void operator()(RenderEnginePrimaryInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_terrain.vert"),
        .frag = VPath("src/shaders/dfr_geometry_terrain.frag")});
};


} // namespace josh::stages::primary
