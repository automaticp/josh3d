#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp" // IWYU pragma: keep (String)
#include "KitchenSink.hpp"
#include <fmt/core.h>
#include <stdexcept>


/*
TODO: We need a way to display errors more nicely.
*/
namespace josh {


/*
A macro so that later we could mass-edit if necessary.
*/
#define JOSH3D_DERIVE_EXCEPTION(Name, ...) \
    JOSH3D_DERIVE_TYPE(Name, __VA_ARGS__)

/*
An exception that defines additional arguments contained in the exception type.

TODO: Make this work for deriving from ancestors that already have args.
Or don't. It's a niche thing anyways.
*/
#define JOSH3D_DERIVE_EXCEPTION_EX(Name, Parent, ...) \
    struct Name : Parent                              \
    {                                                 \
        struct Args __VA_ARGS__ args;                 \
        Name(String message, Args args)               \
            : Parent(MOVE(message))                   \
            , args(MOVE(args))                        \
        {}                                            \
    }


/*
Library-level runtime error class that exists for the sake of
isolation from other subclasses of std::runtime_error.

It's a base for all user-facing exception classes in the library.
*/
JOSH3D_DERIVE_EXCEPTION(RuntimeError, std::runtime_error);


/*
Convenience for single-argument exception types.

TODO: We could add support for types with extra args.
*/
template<typename ErrorT = RuntimeError, typename ...Args>
[[noreturn]]
void throw_fmt(fmt::format_string<Args...> msg, Args&&... args)
{
    throw ErrorT(fmt::format(msg, FORWARD(args)...));
}


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
    throw std::logic_error(message ? message : "Panic raised.");
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
