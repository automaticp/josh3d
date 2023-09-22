#pragma once
#include "GLFenceSync.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "RenderEngine.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "Size.hpp"
#include <chrono>
#include <entt/entt.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <numeric>
#include <cmath>


namespace josh {

class PostprocessHDREyeAdaptationStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_hdr.frag"))
            .get()
    };

    ShaderProgram reduce_sp_{
        ShaderBuilder()
            .load_comp(VPath("src/shaders/pp_hdr_eye_adaptation_screen_reduce.comp"))
            .get()
    };

    SSBOWithIntermediateBuffer<float> reduced_ssbo_{ 0, gl::GL_DYNAMIC_READ };
    size_t old_num_samples_{ 64 };

    FenceSync dispatch_fence_{ FenceSync::create_empty() };

public:
    float current_screen_value{ 1.0f };
    float exposure_factor{ 0.35f };
    float adaptation_rate{ 1.f };
    size_t num_samples{ old_num_samples_ };
    bool use_adaptation{ true };

    PostprocessHDREyeAdaptationStage() noexcept {
        auto dims = dispatch_dimensions(num_samples, 1.0f);
        reduced_ssbo_.bind().create_storage(dims.width * dims.height);
    }

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        if (use_adaptation) {

            const float avg_screen_value = // this frame
                compute_avg_screen_value(engine);

            const float frame_weight =
                engine.frame_timer().delta<float>();

            current_screen_value =
                scaled_weighted_mean_fold(current_screen_value,
                    avg_screen_value, frame_weight, adaptation_rate);
        }

        sp_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0)
                .uniform("use_reinhard", false)
                .uniform("use_exposure", true)
                .uniform("exposure", exposure_function(current_screen_value));

            engine.draw();
        });
    }



private:
    static float scaled_weighted_mean_fold(float current_mean,
        float value, float weight, float scale) noexcept
    {
        // Actually no clue what this does exactly.
        // I needed something similar to a weighted running mean,
        // except I only have the current average and an incoming value,
        // and NO context about any previous values used in calculating the mean.
        //
        // The actual plot of mean vs time for a sudden change
        // of a value looks a lot like the Integrator Circuit's
        // charge/discharge response to a step pulse.
        // So, uhhh, if you're into that kind of thing...
        //
        // The adaptation_rate (passed as scale) is actually pretty consistent
        // across jittery and inconsistent frametimes so that's good.
        //
        // It also looks okay when rendering so I'm not complaining.
        return (current_mean + scale * weight * value) /
            (1 + scale * weight);
    }


    float exposure_function(float screen_value) const noexcept {
        return exposure_factor / (screen_value + 0.0001f);
    }


    float compute_avg_screen_value(const RenderEnginePostprocessInterface& engine) {
        using namespace gl;

        float result_screen_value;

        reduce_sp_.use()
            .and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

                engine.screen_color().bind_to_unit_index(0);
                ashp.uniform("screen_color", 0);

                reduced_ssbo_.bind()
                    .and_then([&, this] {
                        if (dispatch_fence_.id() /* no active fence on the first frame */ &&
                            !dispatch_fence_.has_signaled())
                        {
                            // From some further testing there's a couple more things I have learned:
                            //
                            // - Somehow, even after calling SwapBuffers() and flushing the driver-side
                            //   graphics queue, we can still arrive here with the compute fence unsignaled.
                            //   Either the compute queue is entirely separate on the driver-side (and
                            //   the driver magically knows to insert the fence there and not into the
                            //   graphics queue), or the graphics jobs haven't actually been completed
                            //   from the previous frame until now and I just do not understand something.
                            //
                            // - Neither the fence checking, nor the memory barrier seem to be actually
                            //   necessary to safely read from the buffer. Consequently, reading from
                            //   the buffer acts as a hard synchronization point akin to glFinish().
                            //   The fence appears to be always signaled after a buffer read.
                            //   Not sure if this is implementation-dependent or guaranteed by the spec.
                            //
                            // - The buffer read is expensive with about 1ms cost flat.
                            //   Doing unconditional glFinish() before the read seems to not alter that
                            //   cost. So the read most likely carries the same semantics as glFinish().
                            //
                            // - The compute workload largely doesn't matter for a reasonable number
                            //   of samples (under ~128x128). Rarely do you actually want to dispatch
                            //   more, however. So a buffer read is overwhelming the performance here.
                            //   That's what you're paying for. Sad.
                            //
                            // - Even if you arrive here with the fence signaled, the buffer read retains
                            //   it's cost. Possibly because the driver flushes everything anyways,
                            //   but not sure. More synchronization needed for that read? Is there a PBO
                            //   for buffers?
                            //

                            auto wait_time = std::chrono::milliseconds(
                                static_cast<int64_t>(1000.0 * engine.frame_timer().delta())
                            );
                            SyncWaitResult res =
                                dispatch_fence_.flush_and_wait_for(wait_time);
                            switch (res) {
                                using enum SyncWaitResult;
                                case signaled:
                                case already_signaled:
                                    break;
                                case timeout:
                                    globals::logstream
                                        << "WARN: No signal after " << wait_time.count()
                                        << "ms wait. Calling glFinish.\n";
                                    glFinish();
                                    assert(dispatch_fence_.has_signaled()
                                        && "Dispatch fence hasn't signalled after glFinish. I have no idea, man.");
                                    break;
                                case fail:
                                    assert(false);
                                    break;
                            }
                        }
                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                    })
                    .read_to_storage()
                    .and_then([&, this] {
                        // Do the rest of the reduction on CPU.

                        const auto& storage = reduced_ssbo_.storage();

                        result_screen_value =
                            std::reduce(storage.begin(), storage.end()) / float(storage.size());
                    })
                    .and_then([&, this](BoundSSBOWithIntermediateBuffer<float>& ssbo) {
                        // Dispatch a new compute job.

                        const float aspect_ratio =
                            engine.window_size().aspect_ratio();

                        auto dims =
                            resize_output_storage_if_needed(ssbo, aspect_ratio);

                        glDispatchCompute(dims.width, dims.height, 1);

                        dispatch_fence_.reset();

                    });


            });

        return result_screen_value;
    }

private:
    Size2S dispatch_dimensions(size_t num_y_samples, float aspect_ratio) const noexcept {
        size_t num_x_samples =
            std::ceil(float(num_y_samples) * aspect_ratio);
        return { num_x_samples, num_y_samples };
    }

    Size2S resize_output_storage_if_needed(BoundSSBOWithIntermediateBuffer<float>& ssbo,
        float aspect_ratio) noexcept
    {
        auto old_dims = dispatch_dimensions(old_num_samples_, aspect_ratio);
        auto new_dims = dispatch_dimensions(num_samples,      aspect_ratio);
        if (old_dims != new_dims) {
            ssbo.create_storage(new_dims.width * new_dims.height);
        }
        old_num_samples_ = num_samples;
        return new_dims;
    }

};




} // namespace josh
