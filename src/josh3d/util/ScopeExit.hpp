#pragma once
#include "CommonMacros.hpp"


/*
Fancy template implementations kill compile times for no reason.
*/
namespace josh {


namespace detail {
auto uncaught_exceptions() noexcept -> int;
} // namespace detail

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _ON_SCOPE_EXIT_IMPL(fn, Type, ...) \
    const auto fn = __VA_ARGS__; \
    const struct Type            \
    {                            \
        decltype(fn) f;          \
        ~Type() { f(); }         \
    } Type{ fn }

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _ON_SCOPE_FAIL_IMPL(fn, Type, ...) \
    const auto fn = __VA_ARGS__;           \
    const struct Type                      \
    {                                      \
        int initial_uncaught;              \
        decltype(fn) f;                    \
        ~Type()                            \
        {                                  \
            if (::josh::detail::uncaught_exceptions() > initial_uncaught) f(); \
        }                                  \
    } Type{ ::josh::detail::uncaught_exceptions(), fn }


#define ON_SCOPE_EXIT(...) \
    _ON_SCOPE_EXIT_IMPL(JOSH3D_CONCAT(_scope_exit_fn_, __LINE__), JOSH3D_CONCAT(_scope_exit_, __LINE__), __VA_ARGS__)

#define ON_SCOPE_FAIL(...) \
    _ON_SCOPE_FAIL_IMPL(JOSH3D_CONCAT(_scope_fail_fn_, __LINE__), JOSH3D_CONCAT(_scope_fail_, __LINE__), __VA_ARGS__)

/*
Alternative, single-statement spelling for ON_SCOPE_EXIT(...). Very convenient.
*/
#define DEFER(...) ON_SCOPE_EXIT([&]{ __VA_ARGS__; })


} // namespace josh
