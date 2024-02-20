#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "EnumUtils.hpp"
#include "VPath.hpp"


namespace josh::stages::overlay {


class SSAODebug {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_ssao_debug.frag"))
            .get()
    };

    RawTexture2D<GLConst> noisy_occlusion_;
    RawTexture2D<GLConst> occlusion_;

public:
    enum class OverlayMode : GLint {
        none    = 0,
        noisy   = 1,
        blurred = 2,
    };

    OverlayMode mode{ OverlayMode::none };

    SSAODebug(
        RawTexture2D<GLConst> noisy_occlusion_texture,
        RawTexture2D<GLConst> occlusion_texture
    ) noexcept
        : noisy_occlusion_{ noisy_occlusion_texture }
        , occlusion_      { occlusion_texture }
    {}

    void operator()(RenderEngineOverlayInterface& engine);

};




inline void SSAODebug::operator()(
    RenderEngineOverlayInterface& engine)
{

    if (mode == OverlayMode::none) {
        return;
    }

    noisy_occlusion_.bind_to_unit_index(0);
    occlusion_      .bind_to_unit_index(1);

    sp_.use()
        .uniform("mode", to_underlying(mode))
        .uniform("tex_noisy_occlusion", 0)
        .uniform("tex_occlusion",       1)
        .and_then([&] {
            engine.draw_fullscreen_quad();
        });

}




} // namespace josh::stages::overlay
