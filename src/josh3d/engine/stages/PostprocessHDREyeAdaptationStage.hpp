#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "Size.hpp"
#include <array>
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
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_hdr.frag"))
            .get()
    };

    UniqueShaderProgram reduce_sp_{
        ShaderBuilder()
            .load_comp(VPath("src/shaders/pp_hdr_eye_adaptation_screen_reduce.comp"))
            .get()
    };

    std::array<UniqueSSBO, 3> readback_bufs_;
    size_t current_readback_id_{ 0 };

    RawSSBO<GLConst> current_readback_buffer() const noexcept {
        return readback_bufs_[current_readback_id_];
    }

    RawSSBO<GLConst> previous_readback_buffer() const noexcept {
        const size_t idx =
            (current_readback_id_ + (readback_bufs_.size() - 1)) % readback_bufs_.size();
        return readback_bufs_[idx];
    }

    void resize_all_readback_buffers(const Size2S& new_dims) {
        for (auto& buf : readback_bufs_) {
            buf.bind().allocate_data<float>(new_dims.area(), gl::GL_DYNAMIC_READ);
        }
    }

    void resize_current_readback_buffer(const Size2S& new_dims) {
        readback_bufs_[current_readback_id_].bind()
            .allocate_data<float>(new_dims.area(), gl::GL_DYNAMIC_READ);
    }

    void advance_current_readback_buffer() noexcept {
        current_readback_id_ = (current_readback_id_ + 1) % readback_bufs_.size();
    }

    Size2S old_dispatch_dims_{ dispatch_dimensions(64, 1.f) };

public:
    float  current_screen_value{ 1.0f };
    float  exposure_factor{ 0.35f };
    float  adaptation_rate{ 1.f };
    size_t num_y_samples{ 64 };
    bool   use_adaptation{ true };

    PostprocessHDREyeAdaptationStage() noexcept {
        auto dims = dispatch_dimensions(num_y_samples, 1.0f);
        resize_all_readback_buffers(dims);
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
            engine.screen_color().bind_to_unit_index(0);
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

        advance_current_readback_buffer();

        Size2S dims = dispatch_dimensions(num_y_samples, engine.window_size().aspect_ratio());

        if (old_dispatch_dims_ != dims) {
            resize_current_readback_buffer(dims);
            old_dispatch_dims_ = dims;
        }

        engine.screen_color().bind_to_unit_index(0);
        reduce_sp_.use()
            .uniform("screen_color", 0)
            .and_then([&, this] {
                current_readback_buffer().bind_to_index(0)
                    .and_then([&] {
                        glDispatchCompute(dims.width, dims.height, 1);
                    });
            });



        float result_screen_value{};

        previous_readback_buffer().bind()
            .and_then([&](BoundSSBO<GLConst>& ssbo) {
                std::span<const float> mapped = ssbo.map_for_read<float>();
                result_screen_value =
                    std::reduce(mapped.begin(), mapped.end()) / float(mapped.size());
                ssbo.unmap_current();
            });

        return result_screen_value;
    }

private:
    static Size2S dispatch_dimensions(size_t num_y_samples, float aspect_ratio) noexcept {
        size_t num_x_samples =
            std::ceil(float(num_y_samples) * aspect_ratio);
        return { num_x_samples, num_y_samples };
    }
};




} // namespace josh
