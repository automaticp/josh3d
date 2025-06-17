#pragma once
#include "AABB.hpp"
#include "EnumUtils.hpp"
#include "GLAPICore.hpp"
#include "GPULayout.hpp"
#include "RenderStage.hpp"
#include "ShaderPool.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"


namespace josh {


struct DeferredGeometry
{
    enum class Strategy
    {
        DrawPerMesh, // Naive single draw call for each mesh, rebinding everything between.
        BatchedMDI,  // Batched multidraws, limited by the number of texture units.
        // Bindless, // HAHHAHAHAHAHAH, go patch renderdoc.
    };

    Strategy strategy         = Strategy::DrawPerMesh;
    bool     backface_culling = true;

    // Max number of meshes per multidraw in Batched mode.
    auto max_batch_size() const noexcept -> u32;

    void operator()(RenderEnginePrimaryInterface& engine);


    void _draw_single(RenderEnginePrimaryInterface& engine);

    ShaderToken _sp_single_opaque = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_dsn.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_dsn.frag")});

    ShaderToken _sp_single_atested = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_mat_dsn.vert"),
        .frag = VPath("src/shaders/dfr_geometry_mat_dsn.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));

    struct InstanceDataGPU
    {
        alignas(std430::align_vec4)  mat4   model;
        alignas(std430::align_vec4)  mat3x4 normal_model;
        alignas(std430::align_uint)  u32    object_id;
        alignas(std430::align_float) float  specpower;
    };

    UploadBuffer<InstanceDataGPU>             _instance_data;
    UploadBuffer<DrawElementsIndirectCommand> _mdi_buffer;

    // This limits the number of meshes in a single multidraw.
    auto _max_texture_units() const noexcept -> u32;
    void _draw_batched(RenderEnginePrimaryInterface& engine);

    ShaderToken _sp_batched_opaque = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_dsn_batched.vert"),
        .frag = VPath("src/shaders/dfr_geometry_dsn_batched.frag")},
        ProgramDefines()
            .define("MAX_TEXTURE_UNITS", _max_texture_units()));

    ShaderToken _sp_batched_atested = shader_pool().get({
        .vert = VPath("src/shaders/dfr_geometry_dsn_batched.vert"),
        .frag = VPath("src/shaders/dfr_geometry_dsn_batched.frag")},
        ProgramDefines()
            .define("MAX_TEXTURE_UNITS", _max_texture_units())
            .define("ENABLE_ALPHA_TESTING", 1));
};
JOSH3D_DEFINE_ENUM_EXTRAS(DeferredGeometry::Strategy, DrawPerMesh, BatchedMDI);


} // namespace josh
