#pragma once
#include <cstddef>
#include <cstdint>


/*
Some constants to help deal with layout and alignment requirements of UBOs and SSBOs.
Not complete.

See "7.6.2.2 Standard Uniform Block Layout" in OpenGL spec
https://registry.khronos.org/OpenGL/specs/gl/
*/

namespace josh {


namespace detail {
template<size_t RoundTo>
constexpr size_t round_up_to(size_t value) noexcept {
    const size_t base = value / RoundTo;
    const size_t add  = value % RoundTo ? 1 : 0;
    return (base + add) * RoundTo;
}
} // namespace detail


namespace std430 {


constexpr size_t bmu{ sizeof(char) }; // Basic Machine Unit.

template<typename ScalarT>
constexpr size_t align_scalar{ sizeof(ScalarT) };

constexpr size_t align_float{ align_scalar<float>    };
constexpr size_t align_int  { align_scalar<int32_t>  };
constexpr size_t align_uint { align_scalar<uint32_t> };

template<typename ComponentT, size_t N>
constexpr size_t align_gvec{ detail::round_up_to<2>(N) * align_scalar<ComponentT> };

template<size_t N>
constexpr size_t align_vec{ align_gvec<float, N> };

template<size_t N>
constexpr size_t align_ivec{ align_gvec<int32_t, N> };

template<size_t N>
constexpr size_t align_uvec{ align_gvec<uint32_t, N> };

constexpr size_t align_vec2{ align_vec<2> };
constexpr size_t align_vec3{ align_vec<3> };
constexpr size_t align_vec4{ align_vec<4> };

constexpr size_t align_ivec2{ align_ivec<2> };
constexpr size_t align_ivec3{ align_ivec<3> };
constexpr size_t align_ivec4{ align_ivec<4> };

constexpr size_t align_uvec2{ align_uvec<2> };
constexpr size_t align_uvec3{ align_uvec<3> };
constexpr size_t align_uvec4{ align_uvec<4> };


} // namespace std430
} // namespace josh
