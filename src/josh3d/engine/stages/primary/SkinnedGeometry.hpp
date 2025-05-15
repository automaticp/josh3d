#pragma once
#include "UploadBuffer.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "RenderStage.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"



namespace josh::stages::primary {


class SkinnedGeometry {
public:
    bool enable_backface_culling{ true };

    SkinnedGeometry(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(RenderEnginePrimaryInterface&);

private:
    ShaderToken sp_opaque = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_skinned.vert"),
        .frag = VPath("src/shaders/dfr_geometry_skinned.frag")});

    ShaderToken sp_atested = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_skinned.vert"),
        .frag = VPath("src/shaders/dfr_geometry_skinned.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));

    SharedStorageMutableView<GBuffer> gbuffer_;

    // FIXME: There should be a pool of poses uploaded by the
    // animation system, where the pallete is only referenced
    // by some integral SkeletonID as an index into a sparse set.
    // Or something like that. This would allow us to multidraw
    // skinned meshes.
    UploadBuffer<mat4> skinning_mats_;

};


} // namespace josh::stages::primary
