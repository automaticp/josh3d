#pragma once
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "EnumUtils.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "stages/primary/SSAO.hpp"


namespace josh::stages::overlay {


class SSAODebug {

public:
    enum class OverlayMode : GLint {
        None    = 0,
        Noisy   = 1,
        Blurred = 2,
    };

    OverlayMode mode{ OverlayMode::None };

    SSAODebug(SharedStorageView<AmbientOcclusionBuffers> input_ao) noexcept
        : input_ao_{ std::move(input_ao) }
    {}

    void operator()(RenderEngineOverlayInterface& engine);

private:
    UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_ssao_debug.frag"))
            .get()
    };

    SharedStorageView<AmbientOcclusionBuffers> input_ao_;

};




inline void SSAODebug::operator()(
    RenderEngineOverlayInterface& engine)
{

    if (mode == OverlayMode::None) {
        return;
    }

    input_ao_->noisy_texture  .bind_to_texture_unit(0);
    input_ao_->blurred_texture.bind_to_texture_unit(1);

    sp_->uniform("mode", to_underlying(mode));
    sp_->uniform("tex_noisy_occlusion", 0);
    sp_->uniform("tex_occlusion",       1);

    engine.draw_fullscreen_quad(sp_->use());

}




} // namespace josh::stages::overlay
