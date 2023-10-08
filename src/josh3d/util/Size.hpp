#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <concepts>
#include <cstddef>


namespace josh {


template<typename NumericT>
concept size_representable =
    std::floating_point<NumericT> ||
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);




template<size_representable NumericT>
struct Size2 {
    NumericT width;
    NumericT height;

    constexpr Size2(NumericT width, NumericT height) noexcept
        : width { width  }
        , height{ height }
    {}

    template<size_representable NumericU>
    constexpr explicit Size2(const Size2<NumericU>& other) noexcept
        : Size2{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height)
        }
    {}

    constexpr Size2(const Size2& other) = default; // non-explicit
    constexpr Size2& operator=(const Size2& other) = default;

    template<std::floating_point FloatT = float>
    constexpr FloatT aspect_ratio() const noexcept {
        return
            static_cast<FloatT>(width) /
            static_cast<FloatT>(height);
    }

    // A product of width and height.
    constexpr auto area() const noexcept { return width * height; }

    template<size_representable NumericU>
    constexpr bool operator==(const Size2<NumericU>& other) const noexcept {
        return
            width  == other.width &&
            height == other.height;
    }


};

// Common specializations.
// Closer to OpenGL conventions than to standard library.
using Size2I = Size2<int>;
using Size2U = Size2<unsigned int>;
using Size2S = Size2<size_t>;
using Size2F = Size2<float>;
using Size2D = Size2<double>;




template<size_representable NumericT>
struct Size3 {
    NumericT width;
    NumericT height;
    NumericT depth;

    constexpr Size3(NumericT width, NumericT height, NumericT depth)
        : width { width  }
        , height{ height }
        , depth { depth  }
    {}

    template<size_representable NumericU>
    constexpr explicit Size3(const Size3<NumericU>& other) noexcept
        : Size3{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height),
            static_cast<NumericT>(other.depth)
        }
    {}

    constexpr Size3(const Size3& other) = default; // non-explicit
    constexpr Size3& operator=(const Size3& other) = default;

    template<size_representable NumericU, size_representable NumericY>
    constexpr Size3(const Size2<NumericU>& size2d, NumericY depth) noexcept
        : Size3{
            static_cast<NumericT>(size2d.width),
            static_cast<NumericT>(size2d.height),
            static_cast<NumericT>(depth)
        }
    {}

    template<size_representable NumericU>
    constexpr explicit operator Size2<NumericU>() const noexcept {
        return {
            static_cast<NumericU>(width),
            static_cast<NumericU>(height)
        };
    }

    template<std::floating_point FloatT = float>
    constexpr FloatT aspect_ratio() const noexcept {
        return
            static_cast<FloatT>(width) /
            static_cast<FloatT>(height);
    }

    // A product of width and height.
    constexpr auto slice_area() const noexcept { return width * height; }
    // A product of all three dimensions.
    constexpr auto volume() const noexcept { return width * height * depth; }

    template<size_representable NumericU>
    constexpr bool operator==(const Size3<NumericU>& other) const noexcept {
        return
            width  == other.width  &&
            height == other.height &&
            depth  == other.depth;
    }

};


using Size3I = Size3<int>;
using Size3U = Size3<unsigned int>;
using Size3S = Size3<size_t>;
using Size3F = Size3<float>;
using Size3D = Size3<double>;




} // namespace josh
