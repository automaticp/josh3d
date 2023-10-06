#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "RenderTarget.hpp"
#include <utility>
#include <cstdint>



namespace josh {


template<specialization_of<RenderTarget> TargetT>
class SwapChain {
private:
    std::array<TargetT, 2> bufs_;

    int8_t front_id_{ 0 };
    int8_t back_id_ { 1 };

public:
    SwapChain(TargetT initial_front, TargetT initial_back)
        : bufs_{ std::move(initial_front), std::move(initial_back) }
    {}

    TargetT&       front_target()       noexcept { return bufs_[front_id_]; }
    const TargetT& front_target() const noexcept { return bufs_[front_id_]; }

    TargetT&       back_target()        noexcept { return bufs_[back_id_];  }
    const TargetT& back_target()  const noexcept { return bufs_[back_id_];  }

    void swap_buffers() noexcept {
        std::swap(front_id_, back_id_);
    }

    void resize_all(const Size2I& new_size) {
        front_target().resize_all(new_size);
        back_target() .resize_all(new_size);
    }

    void draw_and_swap(auto&& draw_func) {
        back_target().bind_draw().and_then(draw_func).unbind();
        swap_buffers();
    }

};



} // namespace josh
