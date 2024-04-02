#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "GLAPIBinding.hpp"
#include "RenderTarget.hpp"
#include <utility>
#include <cstdint>



namespace josh {


template<specialization_of<RenderTarget> TargetT>
class SwapChain {
public:
    using render_target_type = TargetT;

private:
    std::array<TargetT, 2> bufs_;

    int8_t front_id_{ 0 };
    int8_t back_id_ { 1 };

public:
    SwapChain(render_target_type initial_front, render_target_type initial_back)
        : bufs_{ std::move(initial_front), std::move(initial_back) }
    {}

    render_target_type&       front_target()       noexcept { return bufs_[front_id_]; }
    const render_target_type& front_target() const noexcept { return bufs_[front_id_]; }

    TargetT&       back_target()        noexcept { return bufs_[back_id_];  }
    const TargetT& back_target()  const noexcept { return bufs_[back_id_];  }

    void swap_buffers() noexcept { std::swap(front_id_, back_id_); }

    Size2I resolution() const noexcept { return front_target().resolution(); }

    void resize(const Size2I& new_resolution) {
        front_target().resize(new_resolution);
        back_target() .resize(new_resolution);
    }

    void resize(GLsizei new_array_elements) {
        front_target().resize(new_array_elements);
        back_target() .resize(new_array_elements);
    }

    void resize(const Size2I& new_resolution, GLsizei new_array_elements) {
        front_target().resize(new_resolution, new_array_elements);
        back_target() .resize(new_resolution, new_array_elements);
    }

    void draw_and_swap(std::invocable<BindToken<Binding::DrawFramebuffer>> auto&& draw_func) {
        auto bind_token = back_target().bind_draw();
        std::invoke(std::forward<decltype(draw_func)>(draw_func), bind_token);
        bind_token.unbind();
        swap_buffers();
    }

};



} // namespace josh
