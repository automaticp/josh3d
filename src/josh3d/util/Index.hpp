#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <cstddef>


// See also Size.hpp


namespace josh {


template<typename NumericT>
concept index_representable =
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);


template<typename NumericT>
concept offset_representable =
    std::floating_point<NumericT> ||
    index_representable<NumericT>;




template<offset_representable NumericT>
struct Offset1 {
    NumericT x{};

    // Implicitly decaying to underlying value.
    constexpr operator NumericT() const noexcept { return x; }

    // It is often desirable to specify null-offset.
    constexpr Offset1() noexcept = default;

    constexpr Offset1(NumericT x) noexcept
        : x{ x }
    {}

    template<offset_representable NumericU>
    constexpr explicit Offset1(const Offset1<NumericU>& other) noexcept
        : Offset1{
            static_cast<NumericT>(other.x),
        }
    {}

    constexpr Offset1(const Offset1& other) = default; // non-explicit
    constexpr Offset1& operator=(const Offset1& other) = default;

    // // Transparently assignable.
    // constexpr Offset1& operator=(const NumericT& x) noexcept { this->x = x; }

    template<offset_representable NumericU>
    constexpr bool operator==(const Offset1<NumericU>& other) const noexcept {
        return x == other.x;
    }

};








template<offset_representable NumericT>
struct Offset2 {
    NumericT x{};
    NumericT y{};

    // It is often desirable to specify null-offset.
    constexpr Offset2() noexcept = default;

    constexpr Offset2(NumericT x, NumericT y) noexcept
        : x{ x }
        , y{ y }
    {}

    template<offset_representable NumericU>
    constexpr explicit Offset2(const Offset2<NumericU>& other) noexcept
        : Offset2{
            static_cast<NumericT>(other.x),
            static_cast<NumericT>(other.y)
        }
    {}

    constexpr Offset2(const Offset2& other) = default; // non-explicit
    constexpr Offset2& operator=(const Offset2& other) = default;

    template<offset_representable NumericU>
    constexpr bool operator==(const Offset2<NumericU>& other) const noexcept {
        return x == other.x && y == other.y;
    }

};




template<offset_representable NumericT>
struct Offset3 {
    NumericT x{};
    NumericT y{};
    NumericT z{};

    constexpr Offset3() noexcept = default;

    constexpr Offset3(NumericT x, NumericT y, NumericT z)
        : x{ x }
        , y{ y }
        , z{ z }
    {}

    template<offset_representable NumericU>
    constexpr explicit Offset3(const Offset3<NumericU>& other) noexcept
        : Offset3{
            static_cast<NumericT>(other.x),
            static_cast<NumericT>(other.y),
            static_cast<NumericT>(other.z)
        }
    {}

    constexpr Offset3(const Offset3& other) = default; // non-explicit
    constexpr Offset3& operator=(const Offset3& other) = default;

    template<offset_representable NumericU, offset_representable NumericY>
    constexpr Offset3(const Offset2<NumericU>& index2d, NumericY z) noexcept
        : Offset3{
            static_cast<NumericT>(index2d.x),
            static_cast<NumericT>(index2d.y),
            static_cast<NumericT>(z)
        }
    {}

    template<offset_representable NumericU>
    constexpr explicit operator Offset2<NumericU>() const noexcept {
        return {
            static_cast<NumericU>(x),
            static_cast<NumericU>(y)
        };
    }

    template<offset_representable NumericU>
    constexpr bool operator==(const Offset3<NumericU>& other) const noexcept {
        return
            x == other.x &&
            y == other.y &&
            z == other.z;
    }

};




// Common specializations.
// Closer to OpenGL conventions than to standard library.

using Offset1I = Offset1<int>;
using Offset1U = Offset1<unsigned int>;
using Offset1S = Offset1<size_t>;
using Offset1F = Offset1<float>;
using Offset1D = Offset1<double>;

template<index_representable NumericT>
using Index1 = Offset1<NumericT>;

using Index1I = Index1<int>;
using Index1U = Index1<unsigned int>;
using Index1S = Index1<size_t>;




using Offset2I = Offset2<int>;
using Offset2U = Offset2<unsigned int>;
using Offset2S = Offset2<size_t>;
using Offset2F = Offset2<float>;
using Offset2D = Offset2<double>;

template<index_representable NumericT>
using Index2 = Offset2<NumericT>;

using Index2I = Index2<int>;
using Index2U = Index2<unsigned int>;
using Index2S = Index2<size_t>;




using Offset3I = Offset3<int>;
using Offset3U = Offset3<unsigned int>;
using Offset3S = Offset3<size_t>;
using Offset3F = Offset3<float>;
using Offset3D = Offset3<double>;

template<index_representable NumericT>
using Index3 = Offset3<NumericT>;

using Index3I = Index3<int>;
using Index3U = Index3<unsigned int>;
using Index3S = Index3<size_t>;




} // namespace josh
