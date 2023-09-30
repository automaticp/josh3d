#pragma once


namespace josh {
namespace detail {

/*
Just a CRTP helper.
*/
template<typename CRTP>
struct AsSelf {
protected:
    using self_type = CRTP;

    CRTP& as_self() noexcept {
        return static_cast<CRTP&>(*this);
    }
    const CRTP& as_self() const noexcept {
        return static_cast<const CRTP&>(*this);
    }
};


} // namespace detail
} // namespace josh
