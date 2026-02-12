#include "HDREyeAdaptation.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "ReadbackBuffer.hpp"
#include "UniformTraits.hpp"
#include "Tracy.hpp"


namespace josh {


HDREyeAdaptation::HDREyeAdaptation(float initial_screen_value)
{
    set_screen_value(initial_screen_value);
}

void HDREyeAdaptation::operator()(PostprocessContext context)
{
    ZSCGPUN("HDREyeAdaptation");
    context.main_front_color_texture().bind_to_texture_unit(0);

    if (use_adaptation)
    {
        if (read_back_exposure)
            _pull_late_exposure();

        _update_intermediate_buffer(context.main_resolution());

        intermediate_buf_->bind_to_index<BufferTargetI::ShaderStorage>(0);

        // Do a first block extraction reduce pass.
        {
            const RawProgram<> sp = sp_block_reduce_.get();
            sp.uniform("screen_color", 0);
            const auto [w, h] = dispatch_dims_;
            glapi::dispatch_compute(sp.use(), w, h, 1);
        }

        // Do a recursive reduction on the intermediate buffer.
        // Take the mean and write the result into the the next_value_buffer().
        {
            const auto sp = sp_recursive_reduce_.get();

            const auto div_ceil = [](auto num, auto denom)
            {
                return num / denom + bool(num % denom);
            };

            value_bufs_.current()->bind_to_index<BufferTargetI::ShaderStorage>(1); // Read
            value_bufs_.next()   ->bind_to_index<BufferTargetI::ShaderStorage>(2); // Write

            const float fold_weight = adaptation_rate * context.frame_timer().delta<float>();

            sp.uniform("mean_fold_weight", fold_weight);
            sp.uniform("block_size",       GLuint(block_size));

            usize  num_workgroups = intermediate_buf_->get_num_elements();
            GLuint dispatch_depth = 0;

            const BindGuard bsp = sp.use();
            do
            {
                num_workgroups = div_ceil(num_workgroups, batch_size);

                sp.uniform("dispatch_depth", dispatch_depth);

                glapi::memory_barrier(BarrierMask::ShaderStorageBit);
                glapi::dispatch_compute(bsp, num_workgroups, 1, 1);

                ++dispatch_depth;
            }
            while (num_workgroups > 1);
            assert(num_workgroups == 1);

            if (read_back_exposure)
            {
                // Fetch the buffer after the write completes.
                late_values_.emplace_front(ReadbackBuffer<float>::fetch(value_bufs_.next()));
            }
        }
    }

    // Do a tonemapping pass.
    {
        const auto sp = sp_tonemap_.get();

        value_bufs_.current()->bind_to_index<BufferTargetI::ShaderStorage>(1);
        sp.uniform("color",           0);
        sp.uniform("value_range",     value_range);
        sp.uniform("exposure_factor", exposure_factor);

        const BindGuard bsp = sp.use();
        glapi::memory_barrier(BarrierMask::ShaderStorageBit);
        context.draw_quad_and_swap(bsp);
    }

    if (use_adaptation)
    {
        value_bufs_.advance();

        if (read_back_exposure)
        {
            // NOTE: This is a rare case where we give "extra lives" to the
            // output, since it is likely to be used by the following frame.
            // We also push a value, not a reference, to avoid lifetime issues
            // in case *this* stage disappears between this and the next frames.
            context.belt().put(exposure, 1);
        }
    }
}

namespace {

auto dispatch_dimensions(usize num_y_samples, float aspect_ratio) noexcept
    -> Size2S
{
    const usize num_x_samples = std::ceil(float(num_y_samples) * aspect_ratio);
    return { num_x_samples, num_y_samples };
}

} // namespace

void HDREyeAdaptation::_update_intermediate_buffer(Extent2I main_resolution)
{
    const Size2S new_dims = dispatch_dimensions(num_y_sample_blocks, main_resolution.aspect_ratio());
    if (dispatch_dims_ != new_dims)
    {
        dispatch_dims_ = new_dims;
        resize_to_fit(intermediate_buf_, dispatch_dims_.area(), { .mode=StorageMode::DynamicServer });
    }
}

auto HDREyeAdaptation::get_screen_value() const noexcept
    -> float
{
    float out;
    value_bufs_.current()->download_data_into({ &out, 1 });
    return out;
}

void HDREyeAdaptation::set_screen_value(float new_value) noexcept
{
    value_bufs_.current()->upload_data({ &new_value, 1 });
}

void HDREyeAdaptation::_pull_late_exposure()
{
    while (not late_values_.is_empty() and late_values_.back().is_available())
    {
        ReadbackBuffer<float> readback = late_values_.pop_back();
        // Emplace the Exposure into the output.
        exposure.latency_in_frames = 1 + readback.times_queried_until_available();
        readback.get_data_into({ &exposure.screen_value, 1 });
        // Compute exposure using the same function as in the tonemap shader.
        exposure.exposure = exposure_factor / (exposure.screen_value + 0.0001f);
        // NOTE: Actually not accurate if you changed exposure_factor between
        // those frames. Sad.

        // And discard the buffer.
    }
}


} // namespace josh
