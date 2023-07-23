#pragma once
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "LightCasters.hpp"
#include "ShadowMappingStage.hpp"
#include "SharedStorage.hpp"
#include "VirtualFilesystem.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>



namespace josh {



class ForwardRenderingStage {
public:
    struct PointShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.08f };
        gl::GLint pcf_samples{ 1 };
        float     pcf_offset{ 0.01f };
        bool      use_fixed_pcf_samples{ true };
    };

    struct DirShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.0015f };
        gl::GLint pcf_samples{ 1 };
    };

private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/in_directional_shadow.vert"))
            .load_frag(VPath("src/shaders/mat_ds_light_adpn_shadow.frag"))
            .get()
    };

    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{
        2, gl::GL_DYNAMIC_DRAW
    };

    using shadow_info_view_t =
        SharedStorageView<ShadowMappingStage::Output>;

    shadow_info_view_t shadow_info_;

public:
    PointShadowParams point_params;
    DirShadowParams   dir_params;

    ForwardRenderingStage(shadow_info_view_t shadow_info)
        : shadow_info_{ std::move(shadow_info) }
    {}

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

private:
    void draw_scene(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    void update_point_light_buffers(const entt::registry& registry);

};





} // namespace josh
