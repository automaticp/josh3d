#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "Index.hpp"
#include <cstddef>


// See also Index.hpp


namespace josh {


template<typename NumericT>
concept size_representable =
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);


template<typename NumericT>
concept extent_representable =
    std::floating_point<NumericT> ||
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);






template<extent_representable NumericT>
struct Extent1 {
    NumericT width;

    operator NumericT() const noexcept { return width; }

    constexpr Extent1(NumericT width) noexcept
        : width { width  }
    {}

    template<extent_representable NumericU>
    constexpr explicit Extent1(const Extent1<NumericU>& other) noexcept
        : Extent1{
            static_cast<NumericT>(other.width),
        }
    {}

    constexpr Extent1(const Extent1& other) = default; // non-explicit
    constexpr Extent1& operator=(const Extent1& other) = default;

    template<extent_representable NumericU>
    constexpr bool operator==(const Extent1<NumericU>& other) const noexcept {
        return width == other.width;
    }

};


template<typename NumT>
constexpr Extent1<NumT> operator-(const Offset1<NumT>& lhs, const Offset1<NumT>& rhs) noexcept {
    return { lhs.x - rhs.x };
}

template<typename NumT>
constexpr Offset1<NumT> operator+(const Offset1<NumT>& lhs, const Extent1<NumT>& rhs) noexcept {
    return { lhs.x + rhs.width };
}

template<typename NumT>
constexpr Offset1<NumT> operator+(const Extent1<NumT>& lhs, const Offset1<NumT>& rhs) noexcept {
    return rhs + lhs;
}






template<extent_representable NumericT>
struct Extent2 {
    NumericT width;
    NumericT height;

    constexpr Extent2(NumericT width, NumericT height) noexcept
        : width { width  }
        , height{ height }
    {}

    template<extent_representable NumericU>
    constexpr explicit Extent2(const Extent2<NumericU>& other) noexcept
        : Extent2{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height)
        }
    {}

    constexpr Extent2(const Extent2& other) = default; // non-explicit
    constexpr Extent2& operator=(const Extent2& other) = default;

    template<std::floating_point FloatT = float>
    constexpr FloatT aspect_ratio() const noexcept {
        return
            static_cast<FloatT>(width) /
            static_cast<FloatT>(height);
    }

    // A product of width and height.
    constexpr auto area() const noexcept { return width * height; }

    template<extent_representable NumericU>
    constexpr bool operator==(const Extent2<NumericU>& other) const noexcept {
        return
            width  == other.width &&
            height == other.height;
    }

};


template<typename NumT>
constexpr Extent2<NumT> operator-(const Offset2<NumT>& lhs, const Offset2<NumT>& rhs) noexcept {
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}

template<typename NumT>
constexpr Offset2<NumT> operator+(const Offset2<NumT>& lhs, const Extent2<NumT>& rhs) noexcept {
    return { lhs.x + rhs.width, lhs.y + rhs.height };
}

template<typename NumT>
constexpr Offset2<NumT> operator+(const Extent2<NumT>& lhs, const Offset2<NumT>& rhs) noexcept {
    return rhs + lhs;
}






template<extent_representable NumericT>
struct Extent3 {
    NumericT width;
    NumericT height;
    NumericT depth;

    constexpr Extent3(NumericT width, NumericT height, NumericT depth)
        : width { width  }
        , height{ height }
        , depth { depth  }
    {}

    template<extent_representable NumericU>
    constexpr explicit Extent3(const Extent3<NumericU>& other) noexcept
        : Extent3{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height),
            static_cast<NumericT>(other.depth)
        }
    {}

    constexpr Extent3(const Extent3& other) = default; // non-explicit
    constexpr Extent3& operator=(const Extent3& other) = default;

    template<extent_representable NumericU, extent_representable NumericY>
    constexpr Extent3(const Extent2<NumericU>& size2d, NumericY depth) noexcept
        : Extent3{
            static_cast<NumericT>(size2d.width),
            static_cast<NumericT>(size2d.height),
            static_cast<NumericT>(depth)
        }
    {}

    template<extent_representable NumericU>
    constexpr explicit operator Extent2<NumericU>() const noexcept {
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

    template<extent_representable NumericU>
    constexpr bool operator==(const Extent3<NumericU>& other) const noexcept {
        return
            width  == other.width  &&
            height == other.height &&
            depth  == other.depth;
    }

};


template<typename NumT>
constexpr Extent3<NumT> operator-(const Offset3<NumT>& lhs, const Offset3<NumT>& rhs) noexcept {
    return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

template<typename NumT>
constexpr Offset3<NumT> operator+(const Offset3<NumT>& lhs, const Extent3<NumT>& rhs) noexcept {
    return { lhs.x + rhs.width, lhs.y + rhs.height, lhs.z + rhs.depth };
}

template<typename NumT>
constexpr Offset3<NumT> operator+(const Extent3<NumT>& lhs, const Offset3<NumT>& rhs) noexcept {
    return rhs + lhs;
}




// Common specializations.
// Closer to OpenGL conventions than to standard library.

using Extent1I = Extent1<int>;
using Extent1U = Extent1<unsigned int>;
using Extent1S = Extent1<size_t>;
using Extent1F = Extent1<float>;
using Extent1D = Extent1<double>;

template<size_representable NumericT>
using Size1 = Extent1<NumericT>;

using Size1I = Size1<int>;
using Size1U = Size1<unsigned int>;
using Size1S = Size1<size_t>;




using Extent2I = Extent2<int>;
using Extent2U = Extent2<unsigned int>;
using Extent2S = Extent2<size_t>;
using Extent2F = Extent2<float>;
using Extent2D = Extent2<double>;

template<size_representable NumericT>
using Size2 = Extent2<NumericT>;

using Size2I = Size2<int>;
using Size2U = Size2<unsigned int>;
using Size2S = Size2<size_t>;




using Extent3I = Extent3<int>;
using Extent3U = Extent3<unsigned int>;
using Extent3S = Extent3<size_t>;
using Extent3F = Extent3<float>;
using Extent3D = Extent3<double>;

template<size_representable NumericT>
using Size3 = Extent3<NumericT>;

using Size3I = Size3<int>;
using Size3U = Size3<unsigned int>;
using Size3S = Size3<size_t>;




} // namespace josh
