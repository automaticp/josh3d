#pragma once
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "MeshRegistry.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"


namespace josh {


struct PointShadowMaps
{
    auto resolution() const noexcept -> Extent2I { return { _side_resolution, _side_resolution }; }
    auto num_cubes()  const noexcept -> i32      { return _cubemaps->get_num_array_elements(); }
    auto cubemaps()   const noexcept -> RawCubemapArray<> { return _cubemaps; }

    // HMM: Trying to save the bandwidth a little. Is this going to work well?
    // We should test performance/quality difference between various formats.
    static constexpr auto iformat = InternalFormat::DepthComponent16;

    // NOTE: Have to store this separately since it's not uncommon for us
    // to have 0 cubes, which means resetting the texture, where we would
    // still like to preserve the resolution.
    i32                _side_resolution = {};
    UniqueCubemapArray _cubemaps;
    void _resize(i32 side_resolution, i32 num_cubes);
};

inline void PointShadowMaps::_resize(i32 side_resolution, i32 num_cubes)
{
    assert(side_resolution > 0);
    if (this->resolution().width != side_resolution or
        this->num_cubes() != num_cubes)
    {
        _cubemaps        = {};
        _side_resolution = side_resolution;
        if (num_cubes > 0)
            _cubemaps->allocate_storage({ side_resolution, side_resolution }, num_cubes, iformat);
    }
}

struct PointShadowView
{
    float z_near;
    float z_far;
    mat4  proj_mat;
    mat4  view_mats[6];
};

struct PointShadows
{
    PointShadowMaps         maps;
    Vector<Entity>          entities; // List of source point light entities. Same order as maps and views.
    Vector<PointShadowView> views;    // Same order as maps.
};


struct PointShadowMapping
{
    // Only resolution N of one side is exposed. The actual cubemap face resolution is NxN.
    auto side_resolution() const noexcept -> i32 { return point_shadows.maps.resolution().width; }
    // The number of cubemaps is controlled by the actual number of ShadowCasting lights.
    auto num_cubes() const noexcept -> i32 { return point_shadows.maps.num_cubes(); }
    void resize_maps(i32 side_resolution);

    PointShadowMapping(i32 side_resolution = 1024);

    void operator()(RenderEnginePrimaryInterface& engine);

    PointShadows point_shadows;

private:
    UniqueFramebuffer fbo_;

    void map_point_shadows(RenderEnginePrimaryInterface& engine);

    void prepare_point_shadows(const Registry& registry);

    void draw_all_world_geometry_with_alpha_test(
        BindToken<Binding::Program>         bound_sp,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const MeshRegistry&                 mesh_registry,
        const Registry&                     registry);

    void draw_all_world_geometry_no_alpha_test(
        BindToken<Binding::Program>         bound_sp,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const MeshRegistry&                 mesh_registry,
        const Registry&                     registry);

    ShaderToken sp_with_alpha_ = shader_pool().get({
        .vert = VPath("src/shaders/depth_cubemap.vert"),
        .geom = VPath("src/shaders/depth_cubemap_array.geom"),
        .frag = VPath("src/shaders/depth_cubemap.frag")},
        ProgramDefines()
            .define("ENABLE_ALPHA_TESTING", 1));

    ShaderToken sp_no_alpha_ = shader_pool().get({
        .vert = VPath("src/shaders/depth_cubemap.vert"),
        .geom = VPath("src/shaders/depth_cubemap_array.geom"),
        .frag = VPath("src/shaders/depth_cubemap.frag")});
};



} // namespace josh::stages::primary
