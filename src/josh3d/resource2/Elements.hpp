#pragma once
#include "AABB.hpp"
#include "EnumUtils.hpp"
#include "Errors.hpp"
#include "Scalars.hpp"
#include <cmath>
#include <glm/gtc/packing.hpp>
#include <limits>


/*
Vocabulary and helpers for working with elements and their layouts.
This comes up a lot when dealing with vertex attributes, animation
keyframes, and other "simple" buffer data.
*/
namespace josh {

enum class ComponentType : u8
{
    u8,
    u8norm,
    i8,
    i8norm,
    u16,
    u16norm,
    i16,
    i16norm,
    u32,
    i32,
    f32,
};
JOSH3D_DEFINE_ENUM_EXTRAS(ComponentType, u8, u8norm, i8, i8norm, u16, u16norm, i16, i16norm, u32, i32, f32);

enum class ComponentKind : u8
{
    signed_int,
    signed_normalized,
    unsigned_int,
    unsigned_normalized,
    floating_point,
};
JOSH3D_DEFINE_ENUM_EXTRAS(ComponentKind, signed_int, signed_normalized, unsigned_int, unsigned_normalized, floating_point);

constexpr auto component_size(ComponentType type) noexcept
    -> usize
{
    switch (type)
    {
        case ComponentType::u8:
        case ComponentType::u8norm:  return sizeof(u8);
        case ComponentType::i8:
        case ComponentType::i8norm:  return sizeof(i8);
        case ComponentType::u16:
        case ComponentType::u16norm: return sizeof(u16);
        case ComponentType::i16:
        case ComponentType::i16norm: return sizeof(i16);
        case ComponentType::u32:     return sizeof(u32);
        case ComponentType::i32:     return sizeof(i32);
        case ComponentType::f32:     return sizeof(float);
    }
    return 0;
}

constexpr auto component_kind(ComponentType type) noexcept
    -> ComponentKind
{
    switch (type)
    {
        case ComponentType::u8:
        case ComponentType::u16:
        case ComponentType::u32:
            return ComponentKind::unsigned_int;
        case ComponentType::u8norm:
        case ComponentType::u16norm:
            return ComponentKind::unsigned_normalized;
        case ComponentType::i8:
        case ComponentType::i16:
        case ComponentType::i32:
            return ComponentKind::signed_int;
        case ComponentType::i8norm:
        case ComponentType::i16norm:
            return ComponentKind::signed_normalized;
        case ComponentType::f32:
            return ComponentKind::floating_point;
    }
    assert(false);
    return {};
}

constexpr auto is_component_normalized(ComponentType type) noexcept
    -> bool
{
    switch (component_kind(type))
    {
        case ComponentKind::signed_normalized:
        case ComponentKind::unsigned_normalized:
            return true;
        default:
            return false;
    }
}

// TODO: We could support matrix layouts also.
enum class ElementLayout : u8
{
    vec1,
    vec2,
    vec3,
    vec4,
};
JOSH3D_DEFINE_ENUM_EXTRAS(ElementLayout, vec1, vec2, vec3, vec4);

constexpr auto component_count(ElementLayout layout) noexcept
    -> usize
{
    switch (layout)
    {
        case ElementLayout::vec1: return 1;
        case ElementLayout::vec2: return 2;
        case ElementLayout::vec3: return 3;
        case ElementLayout::vec4: return 4;
    }
    return 0;
}

struct Element
{
    ComponentType type   : 4;
    ElementLayout layout : 4;
    constexpr auto operator==(const Element&) const noexcept -> bool = default;
    static_assert(josh::enum_size<ComponentType>() <= (1 << 4), "Must fit into 4 bits.");
    static_assert(josh::enum_size<ElementLayout>() <= (1 << 4), "Must fit into 4 bits.");
};

constexpr auto element_size(Element element) noexcept
    -> usize
{
    return component_size(element.type) * component_count(element.layout);
}

namespace detail {

template<ComponentType> struct component_type;
template<> struct component_type<ComponentType::u8>      { using type = u8;    };
template<> struct component_type<ComponentType::u8norm>  { using type = u8;    };
template<> struct component_type<ComponentType::i8>      { using type = i8;    };
template<> struct component_type<ComponentType::i8norm>  { using type = i8;    };
template<> struct component_type<ComponentType::u16>     { using type = u16;   };
template<> struct component_type<ComponentType::u16norm> { using type = u16;   };
template<> struct component_type<ComponentType::i16>     { using type = i16;   };
template<> struct component_type<ComponentType::i16norm> { using type = i16;   };
template<> struct component_type<ComponentType::u32>     { using type = u32;   };
template<> struct component_type<ComponentType::i32>     { using type = i32;   };
template<> struct component_type<ComponentType::f32>     { using type = float; };
template<ComponentType TypeV> using component_type_t = component_type<TypeV>::type;

/*
NOTE: Primary template is defined but calling the funciton
will panic. Use is_element_convertible() to check before calling this.

This is done so that you could instantiate the type in some "generic"
conversion code, but the implementation should never invoke them.
*/
template<ComponentType From, ComponentType To>
struct convert_component_fn
{
    using from_type = component_type_t<From>;
    using to_type   = component_type_t<To>;
    constexpr auto operator()(const from_type&) const
        -> to_type
    {
        panic("Unreachable: Component types are never convertible.");
    }
};

template<ComponentType From, ComponentType To>
requires
    (not is_component_normalized(From)) and
    (not is_component_normalized(To))
struct convert_component_fn<From, To>
{
    using from_type = component_type_t<From>;
    using to_type   = component_type_t<To>;
    constexpr auto operator()(const from_type& v) const noexcept
        -> to_type
    {
        return to_type(v);
    }
};

/*
signed byte
f = max(c / 127.0, -1.0)

unsigned byte
f = c / 255.0

signed short
f = max(c / 32767.0, -1.0)

unsigned short
f = c / 65535.0
*/
template<ComponentType From, ComponentType To>
requires
    (is_component_normalized(From)) and
    (component_kind(To) == ComponentKind::floating_point)
struct convert_component_fn<From, To>
{
    using from_type = component_type_t<From>;
    using to_type   = component_type_t<To>;
    constexpr auto operator()(const from_type& c) const noexcept
        -> to_type
    {
        constexpr to_type hi = std::numeric_limits<from_type>::max();
        if constexpr (component_kind(From) == ComponentKind::signed_normalized)
        {
            return to_type(std::max(c / hi, to_type(-1.0)));
        }
        else
        {
            return to_type(c / hi);
        }
    }
};

/*
signed byte
c = round(f * 127.0)

unsigned byte
c = round(f * 255.0)

signed short
c = round(f * 32767.0)

unsigned short
c = round(f * 65535.0)
*/
template<ComponentType From, ComponentType To>
requires
    (component_kind(From) == ComponentKind::floating_point) and
    (is_component_normalized(To))
struct convert_component_fn<From, To>
{
    using from_type = component_type_t<From>;
    using to_type   = component_type_t<To>;
    constexpr auto operator()(const from_type& f) const noexcept
        -> to_type
    {
        constexpr from_type hi = std::numeric_limits<to_type>::max();
        constexpr from_type lo = std::numeric_limits<to_type>::min();
        // NOTE: Additionally clamping for consistency and safety.
        return to_type(std::clamp(std::round(f * hi), lo, hi));
    }
};

template<ComponentType From, ComponentType To>
constexpr auto convert_component = convert_component_fn<From, To>{};

} // namespace detail

/*
A byte view over a collection of "elements" with a given byte `stride`.
A single element is, for example, a `vec4`, or `float`, or `u16vec2`.
The component type and layout in an element is given by the `element` field.

Note that, technically, each element can alias another when `stride < element_size()`.
This is okay when reading, but likely not desired when writing, as you'll trample over
the previous elements.
*/
struct ElementsView
{
    const void* bytes;         // Read-only bytes.
    usize       element_count; // Number of elements in the view.
    u32         stride;        // Stride in bytes. u32 to pack better.
    Element     element;       // Element type and layout description.
    constexpr explicit operator bool() const noexcept { return bool(bytes); }
};

struct ElementsMutableView
{
    void*   bytes;         // Writable bytes.
    usize   element_count; // Number of elements in the view.
    u32     stride;        // Stride in bytes.
    Element element;       // Element type and layout description.
    constexpr explicit operator bool() const noexcept { return bool(bytes); }
    constexpr operator ElementsView() const noexcept { return { bytes, element_count, stride, element }; }
};

#define _JOSH3D_EACH_ELEMENT_VEC1(type) JOSH3D_EACH_ELEMENT_X(type, vec1)
#define _JOSH3D_EACH_ELEMENT_VEC2(type) JOSH3D_EACH_ELEMENT_X(type, vec2)
#define _JOSH3D_EACH_ELEMENT_VEC3(type) JOSH3D_EACH_ELEMENT_X(type, vec3)
#define _JOSH3D_EACH_ELEMENT_VEC4(type) JOSH3D_EACH_ELEMENT_X(type, vec4)

#define _JOSH3D_EACH_ELEMENT(type)  \
    _JOSH3D_EACH_ELEMENT_VEC1(type) \
    _JOSH3D_EACH_ELEMENT_VEC2(type) \
    _JOSH3D_EACH_ELEMENT_VEC3(type) \
    _JOSH3D_EACH_ELEMENT_VEC4(type)

#define JOSH3D_EACH_ELEMENT       \
    _JOSH3D_EACH_ELEMENT(u8)      \
    _JOSH3D_EACH_ELEMENT(u8norm)  \
    _JOSH3D_EACH_ELEMENT(i8)      \
    _JOSH3D_EACH_ELEMENT(i8norm)  \
    _JOSH3D_EACH_ELEMENT(u16)     \
    _JOSH3D_EACH_ELEMENT(u16norm) \
    _JOSH3D_EACH_ELEMENT(i16)     \
    _JOSH3D_EACH_ELEMENT(i16norm) \
    _JOSH3D_EACH_ELEMENT(u32)     \
    _JOSH3D_EACH_ELEMENT(i32)     \
    _JOSH3D_EACH_ELEMENT(f32)

#define JOSH3D_EACH_ELEMENT_X(type, layout) \
    constexpr Element element_##type##layout = { ComponentType::type, ElementLayout::layout };

JOSH3D_EACH_ELEMENT // This creates a variable for each element value.

#undef JOSH3D_EACH_ELEMENT_X

constexpr auto is_component_convertible(ComponentType from, ComponentType to) noexcept
    -> bool
{
    if (from == to) return true;

    const bool from_normalized =
        is_component_normalized(from);

    const bool to_normalized =
        is_component_normalized(to);

    const bool from_any_int =
        component_kind(from) == ComponentKind::signed_int or
        component_kind(from) == ComponentKind::unsigned_int;

    const bool to_any_int =
        component_kind(to) == ComponentKind::signed_int or
        component_kind(to) == ComponentKind::unsigned_int;

    // Interestingly, conversion between different normalized types is not defined.
    if (from_normalized and to_normalized) return false;
    if (from_any_int and to_normalized)    return false;
    if (from_normalized and to_any_int)    return false;

    return true;
}

/*
Unsafe if can lead to signed integer overflow.
Should not be an issue if the destination type is not signed int.
*/
constexpr auto always_safely_convertible(ComponentType from, ComponentType to) noexcept
    -> bool
{
    if (from == to) return true;
    if (not is_component_convertible(from, to)) return false;

    // Dumb overflow.
    const bool to_signed_int =
        component_kind(to) == ComponentKind::signed_int;

    const bool sized_up =
        component_size(from) < component_size(to);

    const bool from_float =
        component_kind(from) == ComponentKind::floating_point;

    if (to_signed_int and from_float)   return false;
    if (to_signed_int and not sized_up) return false;

    return true;
}

constexpr auto always_safely_convertible(Element from, Element to) noexcept
    -> bool
{
    return always_safely_convertible(from.type, to.type);
}

static_assert(    always_safely_convertible(element_u32vec1, element_u8vec1 ));
static_assert(    always_safely_convertible(element_i8vec1,  element_i16vec1));
static_assert(    always_safely_convertible(element_i32vec1, element_u32vec1)); // Overflow is "safe".
static_assert(    always_safely_convertible(element_i32vec1, element_f32vec1));
static_assert(not always_safely_convertible(element_f32vec1, element_i32vec1));
static_assert(not always_safely_convertible(element_u32vec1, element_i32vec1));
static_assert(not always_safely_convertible(element_i32vec1, element_i16vec1));

/*
The following is considered "lossy":
    - Anything not `always_safely_convertible()`;
    - Unsigned int overflow/underflow;
    - Signed/unsigned int to float if float does not have
    enough bits to fit an integer of that width;
    - Any float to int conversion;
    - Any float to normalized conversion.
*/
constexpr auto always_losslessly_convertible(ComponentType from, ComponentType to) noexcept
    -> bool
{
    if (not always_safely_convertible(from, to)) return false;
    if (from == to) return true;

    const bool same_kind =
        component_kind(from) == component_kind(to);

    const bool sized_down =
        component_size(from) > component_size(to);

    const bool sized_up =
        component_size(from) < component_size(to);

    const bool unsigned_to_signed =
        component_kind(from) == ComponentKind::unsigned_int and
        component_kind(to)   == ComponentKind::signed_int;

    const bool from_int =
        component_kind(from) == ComponentKind::signed_int or
        component_kind(from) == ComponentKind::unsigned_int;

    const bool from_normalized =
        is_component_normalized(from);

    const bool to_float =
        component_kind(to) == ComponentKind::floating_point;

    if (same_kind and not sized_down)    return true;
    if (unsigned_to_signed and sized_up) return true;
    // f32 with 24 signed integer bits can fit i16 and below.
    // f64 with 53 signed integer bits can fit i32 and below (there's no f64 currently).
    if (from_int and to_float and sized_up) return true;
    if (from_normalized and to_float)       return true;

    return false;
}

constexpr auto always_losslessly_convertible(Element from, Element to) noexcept
    -> bool
{
    if (component_count(from.layout) > component_count(to.layout))
        return false;
    return always_losslessly_convertible(from.type, to.type);
}

static_assert(not always_losslessly_convertible(element_u32vec1, element_u8vec1 ));
static_assert(    always_losslessly_convertible(element_i8vec1,  element_i16vec1));
static_assert(not always_losslessly_convertible(element_i32vec1, element_u32vec1));
static_assert(not always_losslessly_convertible(element_i32vec1, element_f32vec1));
static_assert(    always_losslessly_convertible(element_i16vec1, element_f32vec1));
static_assert(    always_losslessly_convertible(element_u16vec1, element_f32vec1));
static_assert(not always_losslessly_convertible(element_i32vec1, element_f32vec1));
static_assert(not always_losslessly_convertible(element_f32vec1, element_i32vec1));
static_assert(not always_losslessly_convertible(element_f32vec1, element_u32vec1));
static_assert(    always_losslessly_convertible(element_u16vec1, element_i32vec1));
static_assert(not always_losslessly_convertible(element_i32vec1, element_i16vec1));
static_assert(not always_losslessly_convertible(element_i32vec4, element_i32vec3));

/*
Will copy min(src.element_count, dst.element_count) elements
from source to destination buffer byte-by-byte.

Returns the number of elements copied.

PRE: `dst.element == src.element`.
PRE: `src` and `dst` ranges do not overlap per-element.
*/
auto copy_elements_raw(
    const ElementsMutableView& dst,
    const ElementsView&        src)
        -> usize;

/*
Will copy one element from source to destination byte-by-byte.

PRE: `dst.element == src.element`.
*/
void copy_one_element_raw(
    void*               dst,
    const ElementsView& src,
    uindex              src_element_idx);

/*
Will copy min(src.element_count, dst.element_count) elements,
explicitly converting them from source to destination format.

Returns the number of elements copied.

NOTE: It is *strongly recommended* to call this function only if
`always_safely_convertible(src.element, dst.element)` is true.
Also consider checking `always_losslessly_convertible(src.element, dst.element)`.

PRE: `is_component_convertible(src.element.type, dst.element.type)`.
PRE: `src` and `dst` ranges do not overlap per-element.
*/
auto copy_convert_elements(
    const ElementsMutableView& dst,
    const ElementsView&        src)
        -> usize;

/*
Will copy one element explicitly converting them from source
to destination format.

NOTE: It is *strongly recommended* to call this function only if
`always_safely_convertible(src.element, dst.element)` is true.
Also consider checking `always_losslessly_convertible(src.element, dst.element)`.

NOTE: This function is likely slower than `copy_elements_*()`
as it has to branch to the exact internal conversion on each call.
Prefer `copy_elements_*()` if possible.

PRE: `is_component_convertible(src.element.type, dst_element.type)`.
*/
void copy_convert_one_element(
    void*               dst,
    Element             dst_element,
    const ElementsView& src,
    uindex              src_element_idx);

/*
Customization trait for user-defined types.
*/
template<typename T, bool Normalized = false> struct element_of;
template<> struct element_of<i8>        { static constexpr auto value = element_i8vec1;  };
template<> struct element_of<u8>        { static constexpr auto value = element_u8vec1;  };
template<> struct element_of<i16>       { static constexpr auto value = element_i16vec1; };
template<> struct element_of<u16>       { static constexpr auto value = element_u16vec1; };
template<> struct element_of<i32>       { static constexpr auto value = element_i32vec1; };
template<> struct element_of<i8,  true> { static constexpr auto value = element_i8normvec1;  };
template<> struct element_of<u8,  true> { static constexpr auto value = element_u8normvec1;  };
template<> struct element_of<i16, true> { static constexpr auto value = element_i16normvec1; };
template<> struct element_of<u16, true> { static constexpr auto value = element_u16normvec1; };
template<> struct element_of<ivec2>     { static constexpr auto value = element_i32vec2; };
template<> struct element_of<ivec3>     { static constexpr auto value = element_i32vec3; };
template<> struct element_of<ivec4>     { static constexpr auto value = element_i32vec4; };
template<> struct element_of<u32>       { static constexpr auto value = element_u32vec1; };
template<> struct element_of<uvec2>     { static constexpr auto value = element_u32vec2; };
template<> struct element_of<uvec3>     { static constexpr auto value = element_u32vec3; };
template<> struct element_of<uvec4>     { static constexpr auto value = element_u32vec4; };
template<> struct element_of<float>     { static constexpr auto value = element_f32vec1; };
template<> struct element_of<vec1>      { static constexpr auto value = element_f32vec1; };
template<> struct element_of<vec2>      { static constexpr auto value = element_f32vec2; };
template<> struct element_of<vec3>      { static constexpr auto value = element_f32vec3; };
template<> struct element_of<vec4>      { static constexpr auto value = element_f32vec4; };
template<> struct element_of<quat>      { static constexpr auto value = element_f32vec4; };
template<typename T, bool Normalized = false> constexpr Element element_of_v = element_of<T>::value;

/*
Minor convenience for types with user-defined `element_of` trait.

PRE: `is_component_convertible(src.element.type, element_of_v<DstT, Normalized>.type)`.
*/
template<typename DstT, bool Normalized = false>
auto copy_convert_one_element(const ElementsView& src, uindex src_element_idx)
    -> DstT
{
    DstT val;
    copy_convert_one_element(&val, element_of_v<DstT, Normalized>, src, src_element_idx);
    return val;
}

} // namespace josh
