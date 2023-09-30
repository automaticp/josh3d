#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "RenderTarget.hpp"
#include <utility>




namespace josh {


template<specialization_of<RenderTarget> TargetT>
class SwapChain {
private:
    TargetT buf1_;
    TargetT buf2_;

    TargetT* front_{ &buf1_ };
    TargetT* back_ { &buf2_ };

public:
    SwapChain(TargetT initial_front, TargetT initial_back)
        : buf1_{ std::move(initial_front) }
        , buf2_{ std::move(initial_back) }
    {}


    TargetT&       front_target()       noexcept { return *front_; }
    const TargetT& front_target() const noexcept { return *front_; }

    TargetT&       back_target()        noexcept { return *back_;  }
    const TargetT& back_target()  const noexcept { return *back_;  }

    void swap_buffers() noexcept {
        std::swap(front_, back_);
    }

    void resize_all(const Size2I& new_size) {
        buf1_.resize_all(new_size);
        buf2_.resize_all(new_size);
    }

    void draw_and_swap(auto&& draw_func) {
        back_target().bind_draw().and_then(draw_func).unbind();
        swap_buffers();
    }



    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    SwapChain(SwapChain&& other) noexcept
        : buf1_{ std::move(*other.front_) }
        , buf2_{ std::move(*other.back_)  }
    {
        // This implementation is somewhat cheating.
        // But might be okay.
    }

    SwapChain& operator=(SwapChain&& other) noexcept {
        buf1_ = std::move(*other.front_);
        buf2_ = std::move(*other.back_);
        front_ = &buf1_;
        back_  = &buf2_;
    }
};



} // namespace josh
