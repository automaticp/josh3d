#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>




namespace josh {


template<typename NumericT>
concept index_representable =
    std::integral<NumericT> &&
    !std::same_as<NumericT, bool>;


template<typename NumericT>
concept offset_representable =
    std::floating_point<NumericT> ||
    index_representable<NumericT>;




template<typename NumericT>
concept size_representable =
    std::integral<NumericT> &&
    !std::same_as<NumericT, bool>;


template<typename NumericT>
concept extent_representable =
    std::floating_point<NumericT> ||
    size_representable<NumericT>;




template<typename NumericT>
concept region_representable =
    offset_representable<NumericT> &&
    extent_representable<NumericT>;





/*
Primary templates.
*/


template<offset_representable NumericT> struct Offset1;
template<offset_representable NumericT> struct Offset2;
template<offset_representable NumericT> struct Offset3;
template<extent_representable NumericT> struct Extent1;
template<extent_representable NumericT> struct Extent2;
template<extent_representable NumericT> struct Extent3;
template<region_representable NumericT> struct Region1;
template<region_representable NumericT> struct Region2;
template<region_representable NumericT> struct Region3;


/*
Common specializations. Closer to OpenGL conventions than to standard library.
*/


using Offset1I = Offset1<int32_t>;
using Offset1U = Offset1<uint32_t>;
using Offset1S = Offset1<size_t>;
using Offset1F = Offset1<float>;
using Offset1D = Offset1<double>;

using Offset2I = Offset2<int32_t>;
using Offset2U = Offset2<uint32_t>;
using Offset2S = Offset2<size_t>;
using Offset2F = Offset2<float>;
using Offset2D = Offset2<double>;

using Offset3I = Offset3<int32_t>;
using Offset3U = Offset3<uint32_t>;
using Offset3S = Offset3<size_t>;
using Offset3F = Offset3<float>;
using Offset3D = Offset3<double>;


template<index_representable NumericT> using Index1 = Offset1<NumericT>;
template<index_representable NumericT> using Index2 = Offset2<NumericT>;
template<index_representable NumericT> using Index3 = Offset3<NumericT>;


using Index1I = Index1<int32_t>;
using Index1U = Index1<uint32_t>;
using Index1S = Index1<size_t>;

using Index2I = Index2<int32_t>;
using Index2U = Index2<uint32_t>;
using Index2S = Index2<size_t>;

using Index3I = Index3<int32_t>;
using Index3U = Index3<uint32_t>;
using Index3S = Index3<size_t>;




using Extent1I = Extent1<int32_t>;
using Extent1U = Extent1<uint32_t>;
using Extent1S = Extent1<size_t>;
using Extent1F = Extent1<float>;
using Extent1D = Extent1<double>;

using Extent2I = Extent2<int32_t>;
using Extent2U = Extent2<uint32_t>;
using Extent2S = Extent2<size_t>;
using Extent2F = Extent2<float>;
using Extent2D = Extent2<double>;

using Extent3I = Extent3<int32_t>;
using Extent3U = Extent3<uint32_t>;
using Extent3S = Extent3<size_t>;
using Extent3F = Extent3<float>;
using Extent3D = Extent3<double>;


template<size_representable NumericT> using Size1 = Extent1<NumericT>;
template<size_representable NumericT> using Size3 = Extent3<NumericT>;
template<size_representable NumericT> using Size2 = Extent2<NumericT>;


using Size1I = Size1<int32_t>;
using Size1U = Size1<uint32_t>;
using Size1S = Size1<size_t>;

using Size2I = Size2<int32_t>;
using Size2U = Size2<uint32_t>;
using Size2S = Size2<size_t>;

using Size3I = Size3<int32_t>;
using Size3U = Size3<uint32_t>;
using Size3S = Size3<size_t>;




using Region1I = Region1<int32_t>;
using Region1U = Region1<uint32_t>;
using Region1S = Region1<size_t>;
using Region1F = Region1<float>;
using Region1D = Region1<double>;

using Region2I = Region2<int32_t>;
using Region2U = Region2<uint32_t>;
using Region2S = Region2<size_t>;
using Region2F = Region2<float>;
using Region2D = Region2<double>;

using Region3I = Region3<int32_t>;
using Region3U = Region3<uint32_t>;
using Region3S = Region3<size_t>;
using Region3F = Region3<float>;
using Region3D = Region3<double>;








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








template<region_representable NumericT>
struct Region1 {
    Offset1<NumericT> offset;
    Extent1<NumericT> extent;
};


template<region_representable NumericT>
struct Region2 {
    Offset2<NumericT> offset;
    Extent2<NumericT> extent;
};


template<region_representable NumericT>
struct Region3 {
    Offset3<NumericT> offset;
    Extent3<NumericT> extent;
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




} // namespace josh
