#pragma once
#include <glbinding/gl/gl.h>


namespace josh {

// A simple transparent wrapper around a uniform location value.
// Mostly used because it's default c-tor sets location to -1.
struct ULocation {
    gl::GLint value{ -1 };

    ULocation() = default;
    ULocation(gl::GLint value) : value{ value } {}

    constexpr operator gl::GLint() const noexcept {
        return value;
    }
};

} // namespace josh
