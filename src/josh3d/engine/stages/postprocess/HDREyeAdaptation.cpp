#include "HDREyeAdaptation.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "Mesh.hpp"
#include "ReadbackBuffer.hpp"


namespace josh::stages::postprocess {
namespace {


auto dispatch_dimensions(size_t num_y_samples, float aspect_ratio) noexcept
    -> Size2S
{
    const size_t num_x_samples = std::ceil(float(num_y_samples) * aspect_ratio);
    return { num_x_samples, num_y_samples };
}


} // namespace



HDREyeAdaptation::HDREyeAdaptation(const Size2I& initial_resolution) noexcept
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


float HDREyeAdaptation::get_screen_value() const noexcept {
    float out;
    value_bufs_.current()->download_data_into({ &out, 1 });
    return out;
}


void HDREyeAdaptation::set_screen_value(float new_value) noexcept {
    value_bufs_.current()->upload_data({ &new_value, 1 });
}


void HDREyeAdaptation::pull_late_exposure() {

    while (!late_values_.is_empty() && late_values_.back().is_available()) {
        ReadbackBuffer<float> readback = late_values_.pop_back();

        // Emplace the Exposure into the output.
        out_->latency_in_frames = 1 + readback.times_queried_until_available();
        readback.get_data_into({ &out_->screen_value, 1 });
        // Compute exposure using the same function as in the tonemap shader.
        out_->exposure = exposure_factor / (out_->screen_value + 0.0001f);
        // NOTE: Actually not accurate if you changed exposure_factor between
        // those frames. Sad.

        // And discard the buffer.
    }

}



void HDREyeAdaptation::operator()(
    RenderEnginePostprocessInterface& engine)
{

    engine.screen_color().bind_to_texture_unit(0);

    if (use_adaptation) {

        if (read_back_exposure) {
            pull_late_exposure();
        }

        Size2S dims = dispatch_dimensions(num_y_sample_blocks, engine.main_resolution().aspect_ratio());
        current_dims_ = dims;

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

            if (read_back_exposure) {
                // Fetch the buffer after the write completes.
                late_values_.emplace_front(ReadbackBuffer<float>::fetch(value_bufs_.next()));
            }
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
