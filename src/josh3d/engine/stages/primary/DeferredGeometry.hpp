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
    // FIXME: This for now does not consider alpha-tested objects properly.
    // So it's off by default.
    bool enable_backface_culling{ false };

    DeferredGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(RenderEnginePrimaryInterface&);


private:
    dsa::UniqueProgram sp_ds{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_ds.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    dsa::UniqueProgram sp_dsn{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_geometry_mat_dsn.vert"))
            .load_frag(VPath("src/shaders/dfr_geometry_mat_dsn.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
        };

    SharedStorageMutableView<GBuffer> gbuffer_;

};


} // namespace josh::stages::primary
