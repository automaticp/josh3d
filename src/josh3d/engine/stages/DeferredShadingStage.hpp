#pragma once
#include "GLShaders.hpp"
#include "GLScalars.hpp"
#include "LightCasters.hpp"
#include "PostprocessRenderer.hpp"
#include "QuadRenderer.hpp"
#include "RenderStage.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "ShadowMappingStage.hpp"
#include "SharedStorage.hpp"
#include "GBuffer.hpp"
#include "VPath.hpp"
#include "stages/CascadedShadowMappingStage.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <utility>


namespace josh {


class DeferredShadingStage {
public:
    struct PointShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.08f };
        GLint   pcf_samples{ 1 };
        GLfloat pcf_offset{ 0.01f };
    };

    struct DirShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.0015f };
        GLint   pcf_samples{ 1 };
        GLfloat pcf_offset{ 1.0f };
    };

private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_shading.vert"))
            .load_frag(VPath("src/shaders/dfr_shading_adpn_shadow_csm.frag"))
            .get()
    };

    ShaderProgram sp_cascade_debug_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_shading.vert"))
            .load_frag(VPath("src/shaders/dfr_shading_csm_debug.frag"))
            .get()
    };

    SharedStorageView<GBuffer> gbuffer_;
    // TODO: Remove and replace with a dedicated point shadow mapper.
    SharedStorageView<ShadowMappingStage::Output> shadow_info_;
    SharedStorageView<CascadedShadowMaps> input_csm_;

    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{
        2, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<CascadeParams> cascade_params_ssbo_{
        3, gl::GL_DYNAMIC_DRAW
    };

    QuadRenderer quad_renderer_;

public:
    PointShadowParams point_params;
    DirShadowParams dir_params;
    bool enable_csm_debug{ false };

    DeferredShadingStage(SharedStorageView<GBuffer> gbuffer,
        SharedStorageView<ShadowMappingStage::Output> shadow_info,
        SharedStorageView<CascadedShadowMaps> input_csm)
        : gbuffer_{ std::move(gbuffer) }
        , shadow_info_{ std::move(shadow_info) }
        , input_csm_{ std::move(input_csm) }
    {}

    void operator()(const RenderEnginePrimaryInterface&,
        const entt::registry&);

private:
    void update_point_light_buffers(const entt::registry& registry);
    void update_cascade_buffer();
    void draw_main(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
    void draw_debug_csm(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
};




} // namespace josh
