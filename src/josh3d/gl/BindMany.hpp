#pragma once
#include "GLScalars.hpp"
#include <tuple>
#include <utility>


namespace josh {


template<typename ...BoundTs>
class BoundMany {
private:
    std::tuple<BoundTs...> bound_;

public:
    BoundMany(BoundTs... bound) : bound_{ std::move(bound)... } {}

    template<std::invocable CallableT>
    auto& and_then(CallableT&& f) noexcept(noexcept(f())) {
        f();
        return *this;
    }

    template<std::invocable CallableT>
    const auto& and_then(CallableT&& f) const noexcept(noexcept(f())) {
        f();
        return *this;
    }

    template<std::invocable<BoundTs&...> CallableT>
    auto& and_then(CallableT&& f) noexcept(noexcept(std::apply(f, bound_))) {
        std::apply(f, bound_);
        return *this;
    }

    template<std::invocable<const BoundTs&...> CallableT>
    const auto& and_then(CallableT&& f) const noexcept(noexcept(std::apply(f, bound_))) {
        std::apply(f, bound_);
        return *this;
    }

    void unbind_all() const noexcept {
        [this]<size_t ...Idx>(std::index_sequence<Idx...>) {
            (std::get<Idx>(bound_).unbind(), ...);
        }(std::make_index_sequence<sizeof...(BoundTs)>());
    }

};




template<typename ...BindableTs>
inline auto bind_many(BindableTs&&... bindables) noexcept {
    /*
    From cppreference (https://en.cppreference.com/w/cpp/language/eval_order):

    9) In list-initialization, every value computation and side effect of a given initializer clause
        is sequenced before every value computation and side effect associated with any initializer
        clause that follows it in the brace-enclosed comma-separated list of initializers.

    We actually need this guarantee, because in some cases order of binding matters (goddamn EBOs).
    */
    return BoundMany{ bindables.bind()... };
}




template<typename BindableT>
struct IBind {
    BindableT& bindable;
    GLuint index;
};




// For textures and samplers.
template<typename ...BindableTs>
inline auto bind_many_to_sampling_units(IBind<BindableTs>... ibind) noexcept {
    return BoundMany{ ibind.bindable.bind_to_unit_index(ibind.index)... };
}


template<typename ...BindableTs>
inline auto bind_many_buffers(IBind<BindableTs>... ibind) noexcept {
    return BoundMany{ ibind.bindable.bind_to_index(ibind.index)... };
}




} // namespace josh
