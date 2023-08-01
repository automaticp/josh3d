#pragma once
#include <utility>
#include <stdexcept>


namespace josh::error {

/*
Library-level runtime error class that exists for the sake of
isolation from other subclasses of std::runtime_error.

Is a base for all exception classes in the library.
*/
class RuntimeError : public std::runtime_error {
public:
    // I am going to regret this prefix thing, aren't I?
    static constexpr auto prefix = "Runtime Error: ";

    // For construction in `throw` statements.
    RuntimeError(std::string msg)
        : RuntimeError(prefix, std::move(msg))
    {}
protected:
    // For pass-through construction in derived classes.
    // I'am so sorry for this design, but I can't seem
    // to come up with anything better.
    RuntimeError(const char* prefix, std::string msg)
        : std::runtime_error(prefix + std::move(msg))
    {}
};


} // namespace josh::error
