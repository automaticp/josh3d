#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "StaticRing.hpp"
#include "VPath.hpp"
#include "Size.hpp"
#include "UniformTraits.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/types.h>
#include <glm/fwd.hpp>
#include <cmath>


namespace josh::stages::postprocess {


class HDREyeAdaptation {
public:
    glm::vec2 value_range{ 0.05f, 10.f };
    float     exposure_factor{ 0.35f };
    float     adaptation_rate{ 1.f   };
    bool      use_adaptation { true  };
    size_t    num_y_sample_blocks{ 64    };

    HDREyeAdaptation(const Size2I& initial_resolution) noexcept;

    // Warning: Slow. Will stall the pipeline.
    float get_screen_value() const noexcept;

    // Warning: Slow. Will stall the pipeline.
    void set_screen_value(float new_value) noexcept;

    Size2S get_sampling_block_dims() const noexcept { return current_dims_; }

    // Values below do not represent the actual number of shader invocations.

    // Num of XY samples in a sampling block in the first pass.
    static constexpr Size2S block_dims{ 8, 8 };

    // Num elements per block/workgroup in the first pass.
    static constexpr size_t block_size = block_dims.area();

    // Num elements per workgroup in the recursive passes.
    static constexpr GLsizeiptr batch_size = 128;


    void operator()(RenderEnginePostprocessInterface& engine);


private:

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

    static Size2S dispatch_dimensions(size_t num_y_samples, float aspect_ratio) noexcept {
        size_t num_x_samples =
            std::ceil(float(num_y_samples) * aspect_ratio);
        return { num_x_samples, num_y_samples };
    }
};




inline HDREyeAdaptation::HDREyeAdaptation(const Size2I& initial_resolution) noexcept
    : intermediate_buf_{
        allocate_buffer<float>(
            NumElems{ dispatch_dimensions(num_y_sample_blocks, initial_resolution.aspect_ratio()).area() },
            StorageMode::StaticServer
        )
    }
    , current_dims_{ dispatch_dimensions(num_y_sample_blocks, initial_resolution.aspect_ratio()) }
{
    set_screen_value(0.2f);
}




inline float HDREyeAdaptation::get_screen_value() const noexcept {
    float out;
    value_bufs_.current()->download_data_into({ &out, 1 });
    return out;
}




inline void HDREyeAdaptation::set_screen_value(float new_value) noexcept {
    value_bufs_.current()->upload_data({ &new_value, 1 });
}




inline void HDREyeAdaptation::operator()(
    RenderEnginePostprocessInterface& engine)
{

    engine.screen_color().bind_to_texture_unit(0);

    if (use_adaptation) {

        Size2S dims = dispatch_dimensions(num_y_sample_blocks, engine.main_resolution().aspect_ratio());


        // Prepare intermediate reduction buffer.
        const auto buf_size = NumElems{ dims.area() };
        resize_to_fit(intermediate_buf_, buf_size);
        intermediate_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);

        // Do a first block extraction reduce pass.
        {
            const auto sp = sp_block_reduce_.get();
            sp.uniform("screen_color", 0);
            glapi::dispatch_compute(sp.use(), dims.width, dims.height, 1);
        }

        // Do a recursive reduction on the intermediate buffer.
        // Take the mean and write the result into the the next_value_buffer().
        {
            const auto sp = sp_recursive_reduce_.get();

            auto div_up = [](auto val, auto denom) {
                bool up = val % denom;
                return val / denom + up;
            };

            value_bufs_.current()->bind_to_index<BufferTargetIndexed::ShaderStorage>(1); // Read
            value_bufs_.next()   ->bind_to_index<BufferTargetIndexed::ShaderStorage>(2); // Write

            const float fold_weight = adaptation_rate * engine.frame_timer().delta<float>();


            sp.uniform("mean_fold_weight", fold_weight);
            sp.uniform("block_size",       GLuint(block_size));

            size_t num_workgroups = buf_size;
            GLuint dispatch_depth{ 0 };

            BindGuard bound_program = sp.use();
            do {
                num_workgroups = div_up(num_workgroups, batch_size);

                sp.uniform("dispatch_depth", dispatch_depth);

                glapi::memory_barrier(BarrierMask::ShaderStorageBit);
                glapi::dispatch_compute(bound_program, num_workgroups, 1, 1);

                ++dispatch_depth;
            } while (num_workgroups > 1);

            assert(num_workgroups == 1);
        }
    }

    // Do a tonemapping pass.
    {
        const auto sp = sp_tonemap_.get();

        value_bufs_.current()->bind_to_index<BufferTargetIndexed::ShaderStorage>(1);
        sp.uniform("color",           0);
        sp.uniform("value_range",     value_range);
        sp.uniform("exposure_factor", exposure_factor);

        BindGuard bound_program = sp.use();
        glapi::memory_barrier(BarrierMask::ShaderStorageBit);
        engine.draw(bound_program);
    }



    if (use_adaptation) {
        value_bufs_.advance();
    }

}




} // namespace josh::stages::postprocess
