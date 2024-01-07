#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <cstddef>


// See also Size.hpp


namespace josh {


template<typename NumericT>
concept index_representable =
    std::floating_point<NumericT> ||
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);




template<index_representable NumericT>
struct Index2 {
    NumericT x;
    NumericT y;

    constexpr Index2(NumericT x, NumericT y) noexcept
        : x{ x }
        , y{ y }
    {}

    template<index_representable NumericU>
    constexpr explicit Index2(const Index2<NumericU>& other) noexcept
        : Index2{
            static_cast<NumericT>(other.x),
            static_cast<NumericT>(other.y)
        }
    {}

    constexpr Index2(const Index2& other) = default; // non-explicit
    constexpr Index2& operator=(const Index2& other) = default;

    template<index_representable NumericU>
    constexpr bool operator==(const Index2<NumericU>& other) const noexcept {
        return x == other.x && y == other.y;
    }

};


// Common specializations.
// Closer to OpenGL conventions than to standard library.
using Index2I = Index2<int>;
using Index2U = Index2<unsigned int>;
using Index2S = Index2<size_t>;
using Index2F = Index2<float>;
using Index2D = Index2<double>;




template<index_representable NumericT>
struct Index3 {
    NumericT x;
    NumericT y;
    NumericT z;

    constexpr Index3(NumericT x, NumericT y, NumericT z)
        : x{ x }
        , y{ y }
        , z{ z }
    {}

    template<index_representable NumericU>
    constexpr explicit Index3(const Index3<NumericU>& other) noexcept
        : Index3{
            static_cast<NumericT>(other.x),
            static_cast<NumericT>(other.y),
            static_cast<NumericT>(other.z)
        }
    {}

    constexpr Index3(const Index3& other) = default; // non-explicit
    constexpr Index3& operator=(const Index3& other) = default;

    template<index_representable NumericU, index_representable NumericY>
    constexpr Index3(const Index2<NumericU>& index2d, NumericY z) noexcept
        : Index3{
            static_cast<NumericT>(index2d.x),
            static_cast<NumericT>(index2d.y),
            static_cast<NumericT>(z)
        }
    {}

    template<index_representable NumericU>
    constexpr explicit operator Index2<NumericU>() const noexcept {
        return {
            static_cast<NumericU>(x),
            static_cast<NumericU>(y)
        };
    }

    template<index_representable NumericU>
    constexpr bool operator==(const Index3<NumericU>& other) const noexcept {
        return
            x == other.x &&
            y == other.y &&
            z == other.z;
    }

};


using Index3I = Index3<int>;
using Index3U = Index3<unsigned int>;
using Index3S = Index3<size_t>;
using Index3F = Index3<float>;
using Index3D = Index3<double>;


} // namespace josh
