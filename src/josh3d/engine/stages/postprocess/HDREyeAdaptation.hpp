#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "Size.hpp"
#include <array>
#include <entt/entt.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/types.h>
#include <cmath>




namespace josh::stages::postprocess {


class HDREyeAdaptation {
private:
    UniqueShaderProgram tonemap_sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_hdr_eye_adaptation_tonemap.frag"))
            .get()
    };

    UniqueShaderProgram block_reduce_sp_{
        ShaderBuilder()
            .load_comp(VPath("src/shaders/pp_hdr_eye_adaptation_sample_image_block.comp"))
            .get()
    };

    UniqueShaderProgram recursive_reduce_sp_{
        ShaderBuilder()
            .load_comp(VPath("src/shaders/pp_hdr_eye_adaptation_recursive_reduce.comp"))
            .get()
    };


    // FIXME: This could really be some kind of RingBuffer<T, N>
    std::array<UniqueSSBO, 2> val_bufs_;
    size_t current_val_id_{ 0 };

    RawSSBO<GLMutable> current_value_buffer() const noexcept {
        return val_bufs_[current_val_id_];
    }

    RawSSBO<GLMutable> next_value_buffer() const noexcept {
        const size_t idx =
            (current_val_id_ + 1) % val_bufs_.size();
        return val_bufs_[idx];
    }

    void advance_current_value_buffer() noexcept {
        current_val_id_ =
            (current_val_id_ + 1) % val_bufs_.size();
    }

    UniqueSSBO intermediate_buf_;
    Size2S old_dims_{ 0, 0 };

    void resize_intermediate_buffer(const Size2S& new_dims) {
        intermediate_buf_.bind()
            .allocate_data<float>(new_dims.area(), gl::GL_DYNAMIC_COPY);
        old_dims_ = new_dims;
    }

public:
    float  exposure_factor{ 0.35f };
    float  adaptation_rate{ 1.f   };
    bool   use_adaptation { true  };
    size_t num_y_sample_blocks{ 64    };

    HDREyeAdaptation() noexcept;

    // Warning: Slow. Will stall the pipeline.
    float get_screen_value() const noexcept;

    // Warning: Slow. Will stall the pipeline.
    void set_screen_value(float new_value) noexcept;

    Size2S get_sampling_block_dims() const noexcept { return old_dims_; }

    // Values below do not represent the actual number of shader invocations.

    // Num of XY samples in a sampling block in the first pass.
    static constexpr Size2S block_dims{ 8, 8 };

    // Num elements per block/workgroup in the first pass.
    static constexpr size_t block_size = block_dims.area();

    // Num elements per workgroup in the recursive passes.
    static constexpr GLsizeiptr batch_size = 128;


    void operator()(RenderEnginePostprocessInterface& engine);

private:
    static Size2S dispatch_dimensions(size_t num_y_samples, float aspect_ratio) noexcept {
        size_t num_x_samples =
            std::ceil(float(num_y_samples) * aspect_ratio);
        return { num_x_samples, num_y_samples };
    }
};




inline HDREyeAdaptation::HDREyeAdaptation() noexcept {
    auto dims = dispatch_dimensions(num_y_sample_blocks, 1.0f);
    resize_intermediate_buffer(dims);

    for (auto& buf : val_bufs_) {
        buf.bind().allocate_data<float>(1, gl::GL_DYNAMIC_COPY);
    }

    set_screen_value(0.2f);
}




inline float HDREyeAdaptation::get_screen_value() const noexcept {
    float out;
    current_value_buffer().bind().get_sub_data_into<float>({ &out, 1 });
    return out;
}




inline void HDREyeAdaptation::set_screen_value(float new_value) noexcept {
    current_value_buffer().bind().sub_data<float>({ &new_value, 1 });
}




inline void HDREyeAdaptation::operator()(
    RenderEnginePostprocessInterface& engine)
{
    using namespace gl;

    engine.screen_color().bind_to_unit_index(0);

    if (use_adaptation) {

        Size2S dims = dispatch_dimensions(num_y_sample_blocks, engine.window_size().aspect_ratio());

        const GLsizeiptr buf_size = dims.area();

        if (old_dims_.area() != buf_size) {
            resize_intermediate_buffer(dims);
        }

        intermediate_buf_.bind_to_index(0);

        // Do a first block extraction reduce pass.
        block_reduce_sp_.use()
            .uniform("screen_color", 0)
            .and_then([&] {
                glDispatchCompute(dims.width, dims.height, 1);
            });


        // Do a recursive reduction on the intermediate buffer.
        // Take the mean and write the result into the the next_value_buffer().

        auto div_up = [](auto val, auto denom) {
            bool up = val % denom;
            return val / denom + up;
        };

        current_value_buffer().bind_to_index(1); // Read
        next_value_buffer()   .bind_to_index(2); // Write

        const float fold_weight = adaptation_rate * engine.frame_timer().delta<float>();

        recursive_reduce_sp_.use()
            .uniform("mean_fold_weight", fold_weight)
            .uniform("block_size",       GLuint(block_size))
            .and_then([&](ActiveShaderProgram<GLMutable>& ashp) {

                size_t num_workgroups = buf_size;
                GLuint dispatch_depth{ 0 };

                do {
                    num_workgroups = div_up(num_workgroups, batch_size);

                    ashp.uniform("dispatch_depth", dispatch_depth);
                    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                    glDispatchCompute(num_workgroups, 1, 1);

                    ++dispatch_depth;
                } while (num_workgroups > 1);

                assert(num_workgroups == 1);
            });


    }

    // Do a tonemapping pass.
    current_value_buffer().bind_to_index(1);
    tonemap_sp_.use()
        .uniform("color", 0)
        .uniform("exposure_factor", exposure_factor)
        .and_then([&] {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            engine.draw();
        });


    if (use_adaptation) {
        advance_current_value_buffer();
    }

}




} // namespace josh::stages::postprocess
