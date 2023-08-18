#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <concepts>


namespace josh {


template<typename NumericT>
concept size_representable =
    std::floating_point<NumericT> ||
    (std::integral<NumericT> && !same_as_remove_cvref<NumericT, bool>);




template<size_representable NumericT>
struct Size2D {
    NumericT width;
    NumericT height;

    Size2D(NumericT width, NumericT height) noexcept
        : width { width  }
        , height{ height }
    {}

    template<size_representable NumericU>
    explicit Size2D(const Size2D<NumericU>& other) noexcept
        : Size2D{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height)
        }
    {}

    Size2D(const Size2D& other) = default; // non-explicit

    template<std::floating_point FloatT = float>
    FloatT aspect_ratio() const noexcept {
        return
            static_cast<FloatT>(width) /
            static_cast<FloatT>(height);
    }

    template<size_representable NumericU>
    bool operator==(const Size2D<NumericU>& other) const noexcept {
        return
            width  == other.width &&
            height == other.height;
    }


};




template<size_representable NumericT>
struct Size3D {
    NumericT width;
    NumericT height;
    NumericT depth;

    Size3D(NumericT width, NumericT height, NumericT depth)
        : width { width  }
        , height{ height }
        , depth { depth  }
    {}

    template<size_representable NumericU>
    explicit Size3D(const Size3D<NumericU>& other) noexcept
        : Size3D{
            static_cast<NumericT>(other.width),
            static_cast<NumericT>(other.height),
            static_cast<NumericT>(other.depth)
        }
    {}

    Size3D(const Size3D& other) = default; // non-explicit

    template<size_representable NumericU, size_representable NumericY>
    Size3D(const Size2D<NumericU>& size2d, NumericY depth) noexcept
        : Size3D{
            static_cast<NumericT>(size2d.width),
            static_cast<NumericT>(size2d.height),
            static_cast<NumericT>(depth)
        }
    {}

    template<size_representable NumericU>
    explicit operator Size2D<NumericU>() const noexcept {
        return {
            static_cast<NumericU>(width),
            static_cast<NumericU>(height)
        };
    }

    template<std::floating_point FloatT = float>
    FloatT aspect_ratio() const noexcept {
        return
            static_cast<FloatT>(width) /
            static_cast<FloatT>(height);
    }

    template<size_representable NumericU>
    bool operator==(const Size3D<NumericU>& other) const noexcept {
        return
            width  == other.width  &&
            height == other.height &&
            depth  == other.depth;
    }

};




} // namespace josh
