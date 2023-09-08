#pragma once
#include "Layout.hpp"
#include "GLScalars.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepthArray.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "CascadeViewsBuilder.hpp"
#include <algorithm>
#include <cassert>
#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>
#include <limits>
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
    RenderTargetDepthArray dir_shadow_maps{ Size3I{ 2048, 2048, 3 } };
    std::vector<CascadeParams> params{ size_t(dir_shadow_maps.size().depth) };
};


/*
CascadeViewsBuilder
|-> CascadeViews
    |-> projection, view; ---------|
    |-> frustum; -> FrustumCuller -|
                                   |-> CascadedeShadowMappingStage
                                       |-> CascadedShadowMaps -> DeferredShadingStage
*/
class CascadedShadowMappingStage {
private:
    SharedStorageView<CascadeViews>   input_;
    SharedStorage<CascadedShadowMaps> output_;

    size_t max_cascades_;

    ShaderProgram sp_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    ShaderProgram sp_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades_))
            .get()
    };

public:
    CascadedShadowMappingStage(
        SharedStorageView<CascadeViews> cascade_info_input,
        size_t max_cascades = 12)
        : input_{ std::move(cascade_info_input) }
        , max_cascades_{ max_cascades }
    {
        assert(input_->cascades.size() < max_cascades_);

        using enum GLenum;
        output_->dir_shadow_maps.depth_target()
            .bind()
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            // Enable shadow sampling with built-in 2x2 PCF
            .set_parameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
            // Comparison: result = ref OPERATOR texture
            // This will return "how much this fragment is lit" from 0 to 1.
            // If you want "how much it's in shadow", use (1.0 - result).
            // Or set the comparison func to GL_GREATER.
            .set_parameter(GL_TEXTURE_COMPARE_FUNC, GL_LESS)
            .unbind();
    }

    SharedStorageView<CascadedShadowMaps> view_output() const noexcept {
        return output_.share_view();
    }

    size_t max_cascades() const noexcept { return max_cascades_; }

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    void resize_maps(Size2I new_size);

private:
    void resize_cascade_storage_if_needed();
    void map_dir_light_shadow_cascade(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);
};




inline void CascadedShadowMappingStage::resize_maps(Size2I new_size) {
    auto& maps = output_->dir_shadow_maps;
    maps.reset_size(Size3I{ new_size, maps.size().depth });
}


} // namespace josh
