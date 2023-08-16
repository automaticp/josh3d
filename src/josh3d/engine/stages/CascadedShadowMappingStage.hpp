#pragma once
#include "Layout.hpp"
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
    alignas(layout::base_alignment_of_float) float     z_split{};
};

// FIXME: figure out proper incapsulation for resizing
// and uploading data, etc.
struct CascadedShadowMaps {
    RenderTargetDepthArray dir_shadow_maps{ 2048, 2048, 3 };
    std::vector<CascadeParams> params{ size_t(dir_shadow_maps.layers()) };
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
public:
    struct Params {

    };

private:
    SharedStorageView<CascadeViews> input_;
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
    }

    SharedStorageView<CascadedShadowMaps> view_output() const noexcept {
        return output_.share_view();
    }

    size_t max_cascades() const noexcept { return max_cascades_; }

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);


private:
    void resize_cascade_storage_if_needed();
    void map_dir_light_shadow_cascade(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);
};


} // namespace josh
