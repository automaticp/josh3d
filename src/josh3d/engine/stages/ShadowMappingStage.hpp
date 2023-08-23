#pragma once
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepth.hpp"
#include "RenderTargetDepthCubemapArray.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <array>
#include <memory>
#include <utility>




namespace josh {


class ShadowMappingStage {
public:
    struct PointShadowParams {
        glm::vec2 z_near_far{ 0.05f, 150.f };
    };

    struct DirShadowParams {
        glm::vec2 z_near_far{ 15.f, 150.f };
        float     projection_scale{ 50.f };
        float     cam_offset{ 100.f };
    };

    struct Output {
        PointShadowParams point_params{};
        DirShadowParams   dir_params{};

        glm::mat4 dir_light_projection_view{};

        RenderTargetDepthCubemapArray point_light_maps{ { 1024, 1024, 0 } };
        RenderTargetDepth dir_light_map{ { 4096, 4096 } };
    };



private:
    ShaderProgram sp_plight_depth_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    ShaderProgram sp_dir_depth_with_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map.vert"))
            .load_frag(VPath("src/shaders/depth_map.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    ShaderProgram sp_plight_depth_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .get()
    };

    ShaderProgram sp_dir_depth_no_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map.vert"))
            .load_frag(VPath("src/shaders/depth_map.frag"))
            .get()
    };


    // ShadowMappingStage fills out the depth maps, and
    // the other stages are given read-only access to
    // the shared shadow map storage and other params.
    SharedStorage<Output> mapping_output_{};


public:
    const PointShadowParams& point_params() const noexcept;
    PointShadowParams& point_params() noexcept;

    const DirShadowParams& dir_params() const noexcept;
    DirShadowParams& dir_params() noexcept;

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);


    auto view_mapping_output() const noexcept
        -> SharedStorageView<Output>
    { return mapping_output_.share_view(); }

    void resize_point_maps(Size2I new_size);
    void resize_dir_map(Size2I new_size);

private:
    void resize_point_light_cubemap_array_if_needed(
        const entt::registry& registry);

    void map_point_light_shadows(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    void map_dir_light_shadows(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);


};






inline auto ShadowMappingStage::point_params() const noexcept
    -> const PointShadowParams&
{ return mapping_output_->point_params; }

inline auto ShadowMappingStage::point_params() noexcept
    -> PointShadowParams&
{ return mapping_output_->point_params; }

inline auto ShadowMappingStage::dir_params() const noexcept
    -> const DirShadowParams&
{ return mapping_output_->dir_params; }

inline auto ShadowMappingStage::dir_params() noexcept
    -> DirShadowParams&
{ return mapping_output_->dir_params; }





} // namespace josh
