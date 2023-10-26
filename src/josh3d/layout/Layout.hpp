#pragma once
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


/*
Some constants to help deal with layout and alignment requirements of UBOs and SSBOs.
Not complete.

See "7.6.2.2 Standard Uniform Block Layout" in OpenGL spec
https://registry.khronos.org/OpenGL/specs/gl/
*/
namespace josh::layout {


template<size_t RoundTo>
constexpr size_t round_up_to(size_t value) noexcept {
    const size_t base = value / RoundTo;
    const size_t add  = value % RoundTo ? 1 : 0;
    return (base + add) * RoundTo;
}

// Always one.
constexpr size_t basic_machine_unit{ sizeof(char) };

template<typename ScalarT>
constexpr size_t base_alignment_of_scalar{ sizeof(ScalarT) };

constexpr size_t base_alignment_of_float{ base_alignment_of_scalar<float> };


template<typename VectorT>
constexpr size_t base_alignment_of_vector{
    round_up_to<2>(VectorT::length()) *
    sizeof(typename VectorT::value_type)
};

constexpr size_t base_alignment_of_vec2 = base_alignment_of_vector<glm::vec2>;
constexpr size_t base_alignment_of_vec3 = base_alignment_of_vector<glm::vec3>;

// Used as a reference alignment when padding arrays in std140:
// Array elements must be aligned to a multiple of this value.
// This is not true in std430.
constexpr size_t base_alignment_of_vec4 = base_alignment_of_vector<glm::vec4>;


} // namespace josh::layout
