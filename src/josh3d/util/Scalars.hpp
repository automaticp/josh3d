#pragma once
#include <cstdint>
#include <cstddef>


/*
There are certain aesthetic reasons to make things less ugly and more consistent.

Prefer these typedefs over the original names.

The following types are used as-is:

    bool
    char
    float
    double

NOTE: Currently not used consistently, but one day...
*/
namespace josh {


using byte    = unsigned char; // I have a 2-hour long lecture about why std::byte was a mistake.
using ubyte   = unsigned char;
using u8      = uint8_t;
using u16     = uint16_t;
using u32     = uint32_t;
using u64     = uint64_t;
using i8      = int8_t;
using i16     = int16_t;
using i32     = int32_t;
using i64     = int64_t;
using uchar   = unsigned char;
using usize   = size_t;    // Unsigned size type.
using uindex  = size_t;    // Unsigned index type.
using idiff   = ptrdiff_t; // Index difference.
using ptrdiff = ptrdiff_t; // Pointer difference.
using uintptr = uintptr_t; // Pointer representation as an unsigned integer.
using i32sz   = i32;       // For compatibility with the GLsizei.


} // namespace josh
