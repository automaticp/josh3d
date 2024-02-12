#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "LightCasters.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "GBufferStorage.hpp"
#include "VPath.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <utility>


namespace josh::stages::primary {


class DeferredShading {
public:
    struct PointShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.08f };
        GLint   pcf_extent{ 1 };
        GLfloat pcf_offset{ 0.01f };
    };

    struct DirShadowParams {
        GLfloat base_bias_tx{ 0.2f };
        bool    blend_cascades{ true };
        GLfloat blend_size_inner_tx{ 50.f };
        GLint   pcf_extent{ 1 };
        GLfloat pcf_offset{ 1.0f };
    };

private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_shading.vert"))
            .load_frag(VPath("src/shaders/dfr_shading_adpn_shadow_csm.frag"))
            .get()
    };

    UniqueShaderProgram sp_cascade_debug_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/dfr_shading.vert"))
            .load_frag(VPath("src/shaders/dfr_shading_csm_debug.frag"))
            .get()
    };

    SharedStorageView<GBuffer>            gbuffer_;
    SharedStorageView<PointShadowMaps>    input_psm_;
    SharedStorageView<CascadedShadowMaps> input_csm_;
    RawTexture2D<GLConst>                 ambient_occlusion_;

    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{
        2, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<CascadeParams> cascade_params_ssbo_{
        3, gl::GL_DYNAMIC_DRAW
    };

public:
    PointShadowParams point_params;
    DirShadowParams dir_params;
    bool use_ambient_occlusion{ true  };
    bool enable_csm_debug     { false };

    DeferredShading(
        SharedStorageView<GBuffer>            gbuffer,
        SharedStorageView<PointShadowMaps>    input_psm,
        SharedStorageView<CascadedShadowMaps> input_csm,
        RawTexture2D<GLConst> ambient_occlusion
    )
        : gbuffer_  { std::move(gbuffer) }
        , input_psm_{ std::move(input_psm) }
        , input_csm_{ std::move(input_csm) }
        , ambient_occlusion_{ ambient_occlusion }
    {}

    void operator()(const RenderEnginePrimaryInterface&,
        const entt::registry&);

private:
    void update_point_light_buffers(const entt::registry& registry);
    void update_cascade_buffer();
    void draw_main(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
    void draw_debug_csm(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
};


} // namespace josh::stages::primary
