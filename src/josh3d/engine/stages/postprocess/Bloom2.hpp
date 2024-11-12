#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"


namespace josh::stages::postprocess {


class Bloom2 {
public:
    bool   enable_bloom    = true;
    float  filter_scale_px = 1.0f;
    float  bloom_weight    = 0.02f;
    // Removes contribution from low-res "wide" mip-levels (3x4, 1x1, etc.)
    // that would otherwise pollute the whole screen from few small bright sources.
    //
    // TODO: This should probably be described by some "min_uv_scale", so that
    // the "max width" of effect could be more controlled.
    size_t max_downsample_levels = 6;

    auto num_available_levels() const noexcept -> size_t;

    void operator()(RenderEnginePostprocessInterface& engine);

private:
    ShaderToken sp_downsample_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_downsample.frag")});

    ShaderToken sp_upsample_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_upsample.frag")});

    ShaderToken sp_apply_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_apply.frag")});

    UniqueSampler sampler_ = []{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    }();

    UniqueSampler screen_sampler_ = []{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

    // RenderTarget is too much of a bother for this.
    UniqueFramebuffer fbo_;
    UniqueTexture2D   bloom_texture_;

};



} // namespace josh::stages::postprocess
