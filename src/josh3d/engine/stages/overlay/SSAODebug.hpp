#pragma once
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
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
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_ssao_debug.frag")});

    SharedStorageView<AmbientOcclusionBuffers> input_ao_;

};




inline void SSAODebug::operator()(
    RenderEngineOverlayInterface& engine)
{

    if (mode == OverlayMode::None) {
        return;
    }

    const auto sp = sp_.get();

    input_ao_->noisy_texture  .bind_to_texture_unit(0);
    input_ao_->blurred_texture.bind_to_texture_unit(1);

    sp.uniform("mode", to_underlying(mode));
    sp.uniform("tex_noisy_occlusion", 0);
    sp.uniform("tex_occlusion",       1);

    engine.draw_fullscreen_quad(sp.use());

}




} // namespace josh::stages::overlay
