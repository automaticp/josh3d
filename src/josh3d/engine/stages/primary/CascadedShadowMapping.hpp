#pragma once
#include "GLTextures.hpp"
#include "Layout.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "stages/precompute/CSMSetup.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/glm.hpp>
#include <cassert>
#include <vector>


namespace josh {


struct CascadeParams {
    alignas(layout::base_alignment_of_vec4)  glm::mat4 projview{};
    alignas(layout::base_alignment_of_vec3)  glm::vec3 scale{};
    alignas(layout::base_alignment_of_float) float     z_split{};
};


// FIXME: figure out proper encapsulation for resizing
// and uploading data, etc.
struct CascadedShadowMaps {
    using CascadesTarget = RenderTarget<UniqueAttachment<Renderable::Texture2DArray>>;

    CascadesTarget dir_shadow_maps_tgt{
        { 2048, 2048 }, 3,
        { InternalFormat::DepthComponent32F }
    };

    std::vector<CascadeParams> params{
        size_t(dir_shadow_maps_tgt.depth_attachment().num_array_elements())
    };
};


} // namespace josh




namespace josh::stages::primary {


/*
CascadeViewsBuilder
|-> CascadeViews
    |-> projection, view; ---------|
    |-> frustum; -> FrustumCuller -|
                                   |-> CascadedeShadowMappingStage
                                       |-> CascadedShadowMaps -> DeferredShadingStage
*/
class CascadedShadowMapping {
public:
    CascadedShadowMapping(
        SharedStorageView<CascadeViews> cascade_info_input,
        size_t                          max_cascades = 12)
        : input_       { std::move(cascade_info_input) }
        , max_cascades_{ max_cascades }
    {
        assert(input_->cascades.size() < max_cascades_);
    }

    auto share_output_view() const noexcept
        -> SharedStorageView<CascadedShadowMaps>
    {
        return output_.share_view();
    }

    auto view_output() const noexcept
        -> const CascadedShadowMaps&
    {
        return *output_;
    }

    size_t max_cascades() const noexcept { return max_cascades_; }

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    SharedStorageView<CascadeViews>   input_;
    SharedStorage<CascadedShadowMaps> output_;

    size_t max_cascades_;

    dsa::UniqueProgram sp_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    dsa::UniqueProgram sp_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .get()
    };


    void resize_cascade_storage_if_needed();

    void map_dir_light_shadow_cascade(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry&               registry);

    void draw_all_world_geometry_no_alpha_test(
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

    void draw_all_world_geometry_with_alpha_test(
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

};




} // namespace josh::stages::primary
