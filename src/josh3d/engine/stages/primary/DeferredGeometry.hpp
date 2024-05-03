#pragma once
#include "stages/primary/GBufferStorage.hpp"
#include "RenderStage.hpp"
#include "GLObjects.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>



namespace josh::stages::primary {


class DeferredGeometry {
public:
    bool enable_backface_culling{ true };

    DeferredGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(RenderEnginePrimaryInterface&);


private:
    UniqueProgram sp_ds_at{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_ds.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_ds.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueProgram sp_ds_noat{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_ds.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_ds.frag"))
            .get()
    };

    UniqueProgram sp_dsn_at{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_dsn.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_dsn.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
        };

    UniqueProgram sp_dsn_noat{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_dsn.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_dsn.frag"))
            .get()
        };


    SharedStorageMutableView<GBuffer> gbuffer_;

};


} // namespace josh::stages::primary
