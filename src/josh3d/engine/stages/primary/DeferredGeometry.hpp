#pragma once
#include "RenderStage.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>



namespace josh::stages::primary {


class DeferredGeometry {
public:
    bool enable_backface_culling{ true };

    void operator()(RenderEnginePrimaryInterface&);

private:
    ShaderToken sp_ds_at = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_ds.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_ds.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));

    ShaderToken sp_ds_noat = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_ds.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_ds.frag")});

    ShaderToken sp_dsn_at = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_dsn.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_dsn.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));

    ShaderToken sp_dsn_noat = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_dsn.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_dsn.frag")});

};


} // namespace josh::stages::primary
