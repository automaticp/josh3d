#pragma once
#include "CategoryCasts.hpp"
#include <fmt/core.h>
#include <stdexcept>


namespace josh {

/*
Panics by throwing a logic_error. Does not cause UB.
TODO: Deprecate in favor of panic().
*/
[[noreturn]]
inline void safe_unreachable() noexcept(false)
{
    throw std::logic_error("Reached unreachable.");
}

/*
Panics by thowing a logic_error with a custom message. Does not cause UB.
TODO: Deprecate in favor of panic().
*/
[[noreturn]]
inline void safe_unreachable(const char* message) noexcept(false)
{
    throw std::logic_error(message);
}

/*
Panics by throwing a logic_error with an optional custom message. Does not cause UB.
*/
[[noreturn]]
inline void panic(const char* message = nullptr) noexcept(false)
{
    throw std::logic_error(message ? message : "Panic.");
}

/*
Panics by throwing a logic_error with a formatted message. Does not cause UB.
*/
template<typename ...Args>
[[noreturn]]
void panic_fmt(fmt::format_string<Args...> msg, Args&&... args)
{
    throw std::logic_error(fmt::format(msg, FORWARD(args)...));
}


} // namespace josh
