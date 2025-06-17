#pragma once
#include "DrawHelpers.hpp"
#include "ECS.hpp"
#include "EnumUtils.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "GPULayout.hpp"
#include "Region.hpp"
#include "RenderEngine.hpp"
#include "Scalars.hpp"
#include "ShaderPool.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"
#include "ViewFrustum.hpp"
#include <cassert>


namespace josh {


struct CascadeView
{
    float         width;         // Orthographic frustum params.
    float         height;        // ''
    float         z_near;        // ''
    float         z_far;         // ''
    vec2          tx_scale;      // Shadowmap texel scale in shadow space.
    FrustumPlanes frustum_world;
    FrustumPlanes frustum_padded_world; // Padded frustum for blending. Used for culling if blending is enabled.
    mat4          view_mat;
    mat4          proj_mat;
};

/*
NOTE: Used mainly in deferred shading and debug.
*/
struct CascadeViewGPU
{
    alignas(std430::align_vec4) mat4 projview = {};
    alignas(std430::align_vec3) vec3 scale    = {};

    // Convinience. Static c-tor to keep this as an aggregate type.
    static auto create_from(const CascadeView& cascade) noexcept
        -> CascadeViewGPU
    {
        const vec3 scale = {
            cascade.width,
            cascade.height,
            cascade.z_far - cascade.z_near
        };
        return {
            .projview = cascade.proj_mat * cascade.view_mat,
            .scale    = scale
        };
    }
};

/*
Draw lists and other draw state kept per cascade.

NOTE: It's convinient to keep this accessible as output for others to look at.
*/
struct CascadeDrawState
{
    Vector<Entity>     drawlist_atested;   // Alpha-tested.
    Vector<Entity>     drawlist_opaque;    //
    UploadBuffer<mat4> world_mats_atested; // Filled out if multidraw is enabled.
    UploadBuffer<mat4> world_mats_opaque;  // "
};

struct CascadeMaps
{
    auto resolution()   const noexcept -> Extent2I { return _textures->get_resolution(); }
    auto num_cascades() const noexcept -> i32      { return _textures->get_num_array_elements(); }
    auto textures()     const noexcept -> RawTexture2DArray<> { return _textures; }

    // TODO: What would happen if this was just 16-bit?
    static constexpr auto iformat = InternalFormat::DepthComponent32F;

    // NOTE: No FBO here as that depends on how *you* draw.
    // That means you'll have to re-attach on every frame.
    UniqueTexture2DArray _textures;
    void _resize(Extent2I resolution, i32 num_cascades);
};

inline void CascadeMaps::_resize(Extent2I resolution, i32 num_cascades)
{
    if (resolution != this->resolution() or
        num_cascades != i32(this->num_cascades()))
    {
        _textures = {};
        _textures->allocate_storage(resolution, num_cascades, iformat);
    }
}

/*
Primary output type. Collection of useful info about the cascades.
*/
struct Cascades
{
    CascadeMaps              maps;
    Vector<CascadeView>      views;
    Vector<CascadeDrawState> drawstates;

    // If false, draw lists were not used. They contain garbage.
    bool  draw_lists_active = false;
    // If true, blending is supported. This does not blend anything by itself,
    // only adjusts the culling to cull correctly in the blend region.
    bool  blend_possible    = false;
    // Size of the blended region. Technically, is a *max* blend size.
    // In texels of the inner cascade when blending any inner-outer pair.
    // TODO: World-space representation of this might be more useful.
    float blend_max_size_inner_tx = 0.f;
};


struct CascadedShadowMapping
{
    enum class Strategy
    {
        SinglepassGS,         // Yeah, let's just duplicate all geometry 5 times, that'll work well.
        PerCascadeCulling,    // Cull per cascade, keeping the total number of drawcalls semi-stable.
        PerCascadeCullingMDI, // Cull per cascade and batch draws with MDI. This is faster, imagine.
    };

    Strategy strategy = Strategy::PerCascadeCulling;

    // Limited in [1, max_cascades()].
    auto num_cascades() const noexcept -> i32;

    // Due to implementation limits.
    auto max_cascades() const noexcept -> i32;

    // Side resolution N of one cascade map. Full resolution is NxN.
    auto side_resolution() const noexcept -> i32;

    // The num_cascades() will not exceed max_cascades().
    // Only square NxN resolution is allowed.
    void resize_maps(i32 side_resolution, i32 num_desired_cascades);

    // Splitting params.
    float split_log_weight = 0.95f;
    float split_bias       = 0.f;

    // Normal GPU vertex culling.
    bool enable_face_culling = true;
    Face faces_to_cull       = Face::Back;

    // This does not blend the cascades by themselves, only adjusts the output
    // of this stage for blending to be correctly supported by the shading stage.
    bool  support_cascade_blending = true;

    // Size of the blending region in texel space of each inner cascade.
    float blend_size_inner_tx      = 50.f;

    CascadedShadowMapping(i32 side_resolution = 2048, i32 num_desired_cascades = 5);

    void operator()(RenderEnginePrimaryInterface& engine);

    // Primary output of this stage.
    Cascades cascades;


    UniqueFramebuffer _fbo;

    auto _allowed_num_cascades(i32 desired_num) const noexcept -> i32;

    void _draw_all_cascades_with_geometry_shader(RenderEnginePrimaryInterface& engine);

    ShaderToken _sp_opaque_singlepass_gs = shader_pool().get({
        .vert = VPath("src/shaders/csm_singlepass.vert"),
        .geom = VPath("src/shaders/csm_singlepass.geom"),
        .frag = VPath("src/shaders/csm_singlepass.frag")},
        ProgramDefines()
            .define("MAX_VERTICES", 3 * max_cascades()));

    ShaderToken _sp_atested_singlepass_gs = shader_pool().get({
        .vert = VPath("src/shaders/csm_singlepass.vert"),
        .geom = VPath("src/shaders/csm_singlepass.geom"),
        .frag = VPath("src/shaders/csm_singlepass.frag")},
        ProgramDefines()
            .define("MAX_VERTICES", 3 * max_cascades())
            .define("ENABLE_ALPHA_TESTING", 1));


    void _draw_with_culling_per_cascade(RenderEnginePrimaryInterface& engine);

    ShaderToken _sp_opaque_per_cascade = shader_pool().get({
        .vert = VPath("src/shaders/depth_map.vert"),
        .frag = VPath("src/shaders/depth_map.frag")});

    ShaderToken _sp_atested_per_cascade = shader_pool().get({
        .vert = VPath("src/shaders/depth_map.vert"),
        .frag = VPath("src/shaders/depth_map.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));


    UploadBuffer<MDICommand> _mdi_buffer;

    ShaderToken _sp_opaque_mdi = shader_pool().get({
        .vert = VPath("src/shaders/csm_opaque_mdi.vert"),
        .frag = VPath("src/shaders/csm_opaque_mdi.frag")});

    ShaderToken _sp_atested_mdi = shader_pool().get({
        .vert = VPath("src/shaders/csm_atested_mdi.vert"),
        .frag = VPath("src/shaders/csm_atested_mdi.frag")},
        ProgramDefines()
            .define("MAX_TEXTURE_UNITS", max_frag_texture_units()));
};
JOSH3D_DEFINE_ENUM_EXTRAS(CascadedShadowMapping::Strategy, SinglepassGS, PerCascadeCulling, PerCascadeCullingMDI);


} // namespace josh
