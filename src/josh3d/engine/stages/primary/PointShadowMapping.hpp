#pragma once
#include "GLAPIBinding.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <unordered_map>


namespace josh::stages::primary {


class PointShadowMapping {
public:

    // Only resolution N of one side is exposed. The actual cubemap face resolution is NxN.
    Size1I side_resolution{ 1024 };

    struct PointShadowInfo {
        // size_t map_idx; // Index into the shadow cubemap array.
    };

    struct PointShadows {
        using Target = RenderTarget<ShareableAttachment<Renderable::CubemapArray>>;
        Target maps;

        // TODO: I couldn't figure out how to store info about each shadowcasting point light...
    };


    PointShadowMapping();
    PointShadowMapping(const Size1I& side_resolution);

    void operator()(RenderEnginePrimaryInterface& engine);

    auto share_output_view() const noexcept -> SharedView<PointShadows> { return product_.share_view(); }
    auto view_output() const noexcept -> const PointShadows& { return *product_; }


private:
    SharedStorage<PointShadows> product_;

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

    void map_point_shadows(RenderEnginePrimaryInterface& engine);

    void resize_cubemap_array_storage_if_needed(
        const entt::registry& registry);

    void draw_all_world_geometry_with_alpha_test(
        BindToken<Binding::Program>         bound_program,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

    void draw_all_world_geometry_no_alpha_test(
        BindToken<Binding::Program>         bound_program,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

};



} // namespace josh::stages::primary
