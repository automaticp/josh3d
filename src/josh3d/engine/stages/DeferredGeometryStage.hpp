#pragma once
#include "GBufferStage.hpp"
#include "GLShaders.hpp"
#include "RenderStage.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>



namespace josh {



class DeferredGeometryStage {
private:
    ShaderProgram sp_ds{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/non_instanced.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_ds.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    ShaderProgram sp_dsn{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_dsn.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_dsn.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
        };

    SharedStorageMutableView<GBuffer> gbuffer_;

public:
    DeferredGeometryStage(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(const RenderEnginePrimaryInterface&, const entt::registry&);

};




} // namespace josh
