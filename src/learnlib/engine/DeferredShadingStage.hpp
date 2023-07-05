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
#include <entt/entity/fwd.hpp>
#include <utility>


namespace learn {


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
            .load_vert("src/shaders/dfr_shading.vert")
            .load_frag("src/shaders/dfr_shading_adpn_shadow.frag")
            .get()
    };

    SharedStorageView<GBuffer> gbuffer_;
    SharedStorageView<ShadowMappingStage::Output> shadow_info_;

    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{
        2, gl::GL_DYNAMIC_DRAW
    };

    QuadRenderer quad_renderer_;

public:
    PointShadowParams point_params;
    DirShadowParams dir_params;

    DeferredShadingStage(SharedStorageView<GBuffer> gbuffer,
        SharedStorageView<ShadowMappingStage::Output> shadow_info)
        : gbuffer_{ std::move(gbuffer) }
        , shadow_info_{ std::move(shadow_info) }
    {}

    void operator()(const RenderEnginePrimaryInterface&,
        const entt::registry&);

private:
    void update_point_light_buffers(const entt::registry& registry);

};




} // namespace learn
