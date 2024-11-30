#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "ReadbackBuffer.hpp"
#include "RenderEngine.hpp"
#include "RingBuffer.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "StaticRing.hpp"
#include "VPath.hpp"
#include "Region.hpp"
#include "UniformTraits.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/types.h>
#include <glm/fwd.hpp>
#include <cmath>


namespace josh {


struct Exposure {
    float  exposure         {};
    float  screen_value     {};
    size_t latency_in_frames{};
};


} // namespace josh



namespace josh::stages::postprocess {


class HDREyeAdaptation {
public:
    glm::vec2 value_range{ 0.05f, 10.f };
    float     exposure_factor{ 0.35f };
    float     adaptation_rate{ 1.f   };
    bool      use_adaptation { true  };
    size_t    num_y_sample_blocks{ 64 };

    // Will produce the exposure used in the tonemapping pass
    // with the latency of 1-2 frames.
    bool read_back_exposure{ true };

    HDREyeAdaptation(const Size2I& initial_resolution) noexcept;

    // Warning: Slow. Will stall the pipeline.
    // This gets you the exact current screen value.
    auto get_screen_value() const noexcept -> float;

    // Warning: Slow. Will stall the pipeline.
    void set_screen_value(float new_value) noexcept;

    auto share_output_view() const noexcept -> SharedView<Exposure> { return out_.share_view(); }
    auto view_output()       const noexcept -> const Exposure&      { return *out_;             }

    auto get_sampling_block_dims() const noexcept -> Size2S { return current_dims_; }

    // Values below do not represent the actual number of shader invocations.

    // Num of XY samples in a sampling block in the first pass.
    static constexpr Size2S block_dims{ 8, 8 };

    // Num elements per block/workgroup in the first pass.
    static constexpr size_t block_size = block_dims.area();

    // Num elements per workgroup in the recursive passes.
    static constexpr GLsizeiptr batch_size = 128;


    void operator()(RenderEnginePostprocessInterface& engine);


private:
    SharedStorage<Exposure> out_;

    ShaderToken sp_tonemap_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_hdr_eye_adaptation_tonemap.frag")});

    ShaderToken sp_block_reduce_ = shader_pool().get({
        .comp = VPath("src/shaders/pp_hdr_eye_adaptation_sample_image_block.comp")});

    ShaderToken sp_recursive_reduce_ = shader_pool().get({
        .comp = VPath("src/shaders/pp_hdr_eye_adaptation_recursive_reduce.comp")});

    // Doublebuffering for the buffers that contain screen values,
    // so that the adaptation shader could write the new screen value
    // while the exposure tonemap shader could use the old one.
    StaticRing<UniqueBuffer<float>, 2> value_bufs_{{
        allocate_buffer<float>(NumElems{ 1 }),
        allocate_buffer<float>(NumElems{ 1 })
    }};


    UniqueBuffer<float> intermediate_buf_;
    Size2S current_dims_;

    BadRingBuffer<ReadbackBuffer<float>> late_values_;
    void pull_late_exposure();

};


} // namespace josh::stages::postprocess
