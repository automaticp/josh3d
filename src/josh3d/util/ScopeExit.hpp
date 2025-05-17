#pragma once
#include "CommonMacros.hpp"


/*
Fancy template implementations kill compile times for no reason.
*/
namespace josh {


#define ON_SCOPE_EXIT(...) \
    const auto JOSH3D_CONCAT(_scope_exit_fn_, __LINE__) = __VA_ARGS__; \
    const struct JOSH3D_CONCAT(_scope_exit_, __LINE__) {               \
        decltype(JOSH3D_CONCAT(_scope_exit_fn_, __LINE__)) f;          \
        ~JOSH3D_CONCAT(_scope_exit_, __LINE__)() { f(); }              \
    } JOSH3D_CONCAT(_scope_exit_, __LINE__){                           \
        JOSH3D_CONCAT(_scope_exit_fn_, __LINE__)                       \
    }


namespace detail {
auto uncaught_exceptions() noexcept -> int;
} // namespace detail


#define ON_SCOPE_FAIL(...) \
    const auto JOSH3D_CONCAT(_scope_fail_fn_, __LINE__) = __VA_ARGS__;         \
    const struct JOSH3D_CONCAT(_scope_fail_, __LINE__) {                       \
        int initial_uncaught;                                                  \
        decltype(JOSH3D_CONCAT(_scope_fail_fn_, __LINE__)) f;                  \
        ~JOSH3D_CONCAT(_scope_fail_, __LINE__)() {                             \
            if (::josh::detail::uncaught_exceptions() > initial_uncaught) f(); \
        }                                                                      \
    } JOSH3D_CONCAT(_scope_fail_, __LINE__){                                   \
        ::josh::detail::uncaught_exceptions(),                                 \
        JOSH3D_CONCAT(_scope_fail_fn_, __LINE__)                               \
    }


} // namespace josh
