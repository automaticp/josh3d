#pragma once
#include "stages/primary/GBufferStorage.hpp"
#include "GLShaders.hpp"
#include "RenderStage.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>



namespace josh::stages::primary {


class DeferredGeometry {
private:
    UniqueShaderProgram sp_ds{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_ds.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueShaderProgram sp_dsn{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_dsn.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_dsn.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
        };

    SharedStorageMutableView<GBuffer> gbuffer_;

public:
    DeferredGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(RenderEnginePrimaryInterface&);

};


} // namespace josh::stages::primary
