#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <entt/entity/fwd.hpp>
#include <utility>


namespace josh::stages::primary {


class TerrainGeometry {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_terrain.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_terrain.frag"))
            .get()
    };

    SharedStorageMutableView<GBuffer> gbuffer_;

public:
    TerrainGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
};


} // namespace josh::stages::primary
