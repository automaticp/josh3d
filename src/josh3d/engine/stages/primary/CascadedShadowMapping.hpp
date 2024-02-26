#pragma once
#include "Layout.hpp"
#include "GLScalars.hpp"
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


// FIXME: figure out proper incapsulation for resizing
// and uploading data, etc.
struct CascadedShadowMaps {
    using CascadesTarget = RenderTarget<UniqueAttachment<RawTexture2DArray>>;
private:
    static CascadesTarget make_cascades_target(const Size3I& initial_size) {
        using enum GLenum;
        CascadesTarget tgt{
            { initial_size, { GL_DEPTH_COMPONENT32F } }
        };
        tgt.depth_attachment().texture().bind()
            .set_border_color({ 1.f, 1.f, 1.f, 1.f })
            .set_wrap_st(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)
            .set_min_mag_filters(GL_LINEAR, GL_LINEAR)
            // Enable shadow sampling with built-in 2x2 PCF
            .set_parameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
            // Comparison: result = ref OPERATOR texture
            // This will return "how much this fragment is lit" from 0 to 1.
            // If you want "how much it's in shadow", use (1.0 - result).
            // Or set the comparison func to GL_GREATER.
            .set_parameter(GL_TEXTURE_COMPARE_FUNC, GL_LESS)
            .unbind();

        return tgt;
    }

public:
    CascadesTarget dir_shadow_maps_tgt{ make_cascades_target({ 2048, 2048, 3 }) };
    std::vector<CascadeParams> params{ size_t(dir_shadow_maps_tgt.depth_attachment().size().depth) };
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
private:
    SharedStorageView<CascadeViews>   input_;
    SharedStorage<CascadedShadowMaps> output_;

    size_t max_cascades_;

    UniqueShaderProgram sp_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueShaderProgram sp_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .get()
    };

public:
    CascadedShadowMapping(
        SharedStorageView<CascadeViews> cascade_info_input,
        size_t max_cascades = 12)
        : input_{ std::move(cascade_info_input) }
        , max_cascades_{ max_cascades }
    {
        assert(input_->cascades.size() < max_cascades_);
    }

    SharedStorageView<CascadedShadowMaps> share_output_view() const noexcept {
        return output_.share_view();
    }

    const CascadedShadowMaps& view_output() const noexcept {
        return *output_;
    }

    size_t max_cascades() const noexcept { return max_cascades_; }

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    void resize_cascade_storage_if_needed();
    void map_dir_light_shadow_cascade(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);
};




} // namespace josh::stages::primary
