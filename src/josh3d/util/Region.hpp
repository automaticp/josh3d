#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "Index.hpp"
#include "Size.hpp"
#include <cstddef>



namespace josh {

template<typename NumericT>
concept region_representable =
    offset_representable<NumericT> && extent_representable<NumericT>;



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




using Region1I = Region1<int>;
using Region1U = Region1<unsigned int>;
using Region1S = Region1<size_t>;
using Region1F = Region1<float>;
using Region1D = Region1<double>;

using Region2I = Region2<int>;
using Region2U = Region2<unsigned int>;
using Region2S = Region2<size_t>;
using Region2F = Region2<float>;
using Region2D = Region2<double>;

using Region3I = Region3<int>;
using Region3U = Region3<unsigned int>;
using Region3S = Region3<size_t>;
using Region3F = Region3<float>;
using Region3D = Region3<double>;




} // namespace josh
