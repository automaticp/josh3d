#pragma once
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <entt/entity/fwd.hpp>
#include <utility>


namespace josh::stages::primary {


class TerrainGeometry {
public:
    TerrainGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_terrain.vert"),
        .frag = VPath("src/shaders/dfr_geometry_terrain.frag")});

    SharedStorageMutableView<GBuffer> gbuffer_;

};


} // namespace josh::stages::primary
