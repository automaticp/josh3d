#include "Elements.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include <type_traits>


namespace josh {
namespace {

/*
NOTE: Does not assert for always_safely_convertible() as the specific
values *can* be safely convertible, even if the full range is not.
*/
template<Element D, Element S>
void convert_element(void* dst, const void* src) noexcept
{
    constexpr usize dst_size  = element_size(D);
    constexpr usize src_size  = element_size(S);
    constexpr usize dst_count = component_count(D.layout);
    constexpr usize src_count = component_count(S.layout);
    constexpr usize min_count = std::min(dst_count, src_count);
    using dst_type = detail::component_type_t<D.type>;
    using src_type = detail::component_type_t<S.type>;
    constexpr auto convert_component = detail::convert_component<S.type, D.type>;

    dst_type dst_values[dst_count] = {}; // Zero init if missing in src.
    src_type src_values[src_count];

    std::memcpy(src_values, src, src_size);

    for (const uindex i : irange(min_count))
        dst_values[i] = convert_component(src_values[i]);

    std::memcpy(dst, dst_values, dst_size);
}

/*
This is an unsafe helper with compile-time element values.
You probably want a big runtime switch instead.

NOTE: Using byte pointers because alignment is not guaranteed.

PRE: `dst` and `src` form valid ranges.
PRE: `dst` and `src` ranges do not overlap per-element.
PRE: `is_component_convertible(S.type, D.type)`.
*/
template<Element D, Element S>
void convert_elements(
    usize        element_count,
    ubyte*       dst,
    usize        dst_stride,
    const ubyte* src,
    usize        src_stride) noexcept
{
    for (const uindex _ : irange(element_count))
    {
        convert_element<D, S>(dst, src);
        src += src_stride;
        dst += dst_stride;
    }
}

} // namespace

auto copy_elements_raw(
    const ElementsMutableView& dst,
    const ElementsView&        src)
        -> usize
{
    assert(dst.element == src.element);

    const ubyte* src_ptr   = (const ubyte*)src.bytes;
    ubyte*       dst_ptr   = (ubyte*)      dst.bytes;
    const usize  min_count = std::min(src.element_count, dst.element_count);

    for (const uindex _ : irange(min_count))
    {
        std::memcpy(dst_ptr, src_ptr, element_size(src.element));
        src_ptr += src.stride;
        dst_ptr += dst.stride;
    }

    return min_count;
}

void copy_one_element_raw(
    void*               dst_bytes,
    const ElementsView& src,
    uindex              src_element_idx)
{
    const ElementsMutableView dst_one = {
        .bytes         = dst_bytes,
        .element_count = 1,
        .stride        = u32(element_size(src.element)),
        .element       = src.element,
    };

    const ElementsView src_one = {
        .bytes         = (const ubyte*)src.bytes + src_element_idx * src.stride,
        .element_count = 1,
        .stride        = src.stride,
        .element       = src.element,
    };

    copy_elements_raw(dst_one, src_one);
}

template<uindex I>
using comptime_idx = std::integral_constant<uindex, I>;

auto copy_convert_elements(
    const ElementsMutableView& dst,
    const ElementsView&        src)
        -> usize
{
    const usize min_count = std::min(dst.element_count, src.element_count);
    auto dst_bytes = (ubyte*)      dst.bytes;
    auto src_bytes = (const ubyte*)src.bytes;

    // There are (4*11)^2 = 44^2 = 1936 combinations of all src and dst elements.
    // HMM: We can cut down on this significantly if we take layout as a runtime value.
    // Well, this is going to be fun either way...

#define JOSH3D_EACH_ELEMENT_X(type, layout) element_##type##layout,

    constexpr Element elements[] = { JOSH3D_EACH_ELEMENT };
    constexpr usize   N          = std::size(elements);

#undef JOSH3D_EACH_ELEMENT_X

    auto body = [&]<uindex I, uindex J>(comptime_idx<I>, comptime_idx<J>)
    {
        constexpr auto dst_element = elements[I];
        constexpr auto src_element = elements[J];
        if (dst.element == dst_element and src.element == src_element)
            convert_elements<dst_element, src_element>(
                min_count, dst_bytes, dst.stride, src_bytes, src.stride);
    };

    // You know things are bad when you need both an X-macro *and* a
    // doubly-nested "expansion statement". Reflection when already?

    // Rest-in-peace, compiler. You did well in this life.
    EXPAND(I, N, &)
    {
        EXPAND(J, N, &)
        {
            ((void(I),
                ((void(J),
                    body(comptime_idx<I>{}, comptime_idx<J>{})
                ), ...)
            ), ...);
        };
    };

    // Who said the compiler will even *try* to optimize those if statements?

    return min_count;
}

void copy_convert_one_element(
    void*               dst_bytes,
    Element             dst_element,
    const ElementsView& src,
    uindex              src_element_idx)
{
    const ElementsMutableView dst_one = {
        .bytes         = dst_bytes,
        .element_count = 1,
        .stride        = u32(element_size(dst_element)),
        .element       = dst_element,
    };

    const ElementsView src_one = {
        .bytes         = (const ubyte*)src.bytes + src_element_idx * src.stride,
        .element_count = 1,
        .stride        = src.stride,
        .element       = src.element,
    };

    copy_convert_elements(dst_one, src_one);
}


} // namespace josh
