#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "ReadbackBuffer.hpp"
#include "RenderEngine.hpp"
#include "RingBuffer.hpp"
#include "Scalars.hpp"
#include "ShaderPool.hpp"
#include "StaticRing.hpp"
#include "VPath.hpp"
#include "Region.hpp"


namespace josh {


struct FrameExposure
{
    float exposure          = {};
    float screen_value      = {};
    usize latency_in_frames = {};
    // TODO: latency_in_ms ?
};


/*
Combined tonemap and "eye adaptation" pass.
Can also output computed exposure with a tiny latency.
*/
struct HDREyeAdaptation
{
    vec2  value_range         = { 0.05f, 10.f };
    float exposure_factor     = 0.35f;
    bool  use_adaptation      = true;
    float adaptation_rate     = 1.f;
    usize num_y_sample_blocks = 64;

    // Will produce the FrameExposure used in the tonemapping pass with the latency of 1-2 frames.
    bool read_back_exposure = true;

    // WARN: Slow. Will stall the pipeline.
    // This gets you the exact current screen value.
    auto get_screen_value() const noexcept -> float;
    // WARN: Slow. Will stall the pipeline.
    void set_screen_value(float new_value) noexcept;

    // TODO: What is this for?
    auto get_sampling_block_dims() const noexcept -> Size2S { return dispatch_dims_; }

    HDREyeAdaptation(float initial_screen_value = 0.2f);

    void operator()(RenderEnginePostprocessInterface& engine);

    FrameExposure exposure; // Latest exposure from a few frames back.

    // Values below do not represent the actual number of shader invocations.
    // Those will depend on the screen resolution aswell.
    static constexpr Size2S block_dims = { 8, 8 };          // Num of XY samples in a sampling block in the first pass.
    static constexpr usize  block_size = block_dims.area(); // Num elements per block/workgroup in the first pass.
    static constexpr usize  batch_size = 128;               // Num elements per workgroup in the recursive passes.

private:
    // Doublebuffering for the buffers that contain screen values,
    // so that the adaptation shader could write the new screen value
    // while the exposure tonemap shader could use the old one.
    StaticRing<UniqueBuffer<float>, 2> value_bufs_ = {
        allocate_buffer<float>(1),
        allocate_buffer<float>(1)
    };

    UniqueBuffer<float> intermediate_buf_;
    Size2S              dispatch_dims_ = { 0, 0 };
    void _update_intermediate_buffer(Extent2I main_resolution);

    BadRingBuffer<ReadbackBuffer<float>> late_values_;
    void _pull_late_exposure();

    ShaderToken sp_tonemap_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_hdr_eye_adaptation_tonemap.frag")});

    ShaderToken sp_block_reduce_ = shader_pool().get({
        .comp = VPath("src/shaders/pp_hdr_eye_adaptation_sample_image_block.comp")});

    ShaderToken sp_recursive_reduce_ = shader_pool().get({
        .comp = VPath("src/shaders/pp_hdr_eye_adaptation_recursive_reduce.comp")});
};


} // namespace josh
