#pragma once
#include "Scalars.hpp"
#include <boost/any/basic_any.hpp>
#include <boost/any/unique_any.hpp>


/*
TODO: Move AnyRef stuff here probably. And tie it to the owners.
*/
namespace josh {


/*
NOTE: Alignment is suggestive. If alignment of a type is stricter, it will be lifted to heap.
*/
template<
    usize SBOSizeV  = 3 * sizeof(void*),
    usize SBOAlignV = alignof(void*)
>
using Any = boost::anys::basic_any<SBOSizeV, SBOAlignV>;

/*
NOTE: This always heap-allocates. This is to ensure that immovable types can be stored too.
Potentially, we might need some MoveOnlyAny for SBO optimized move-only types.
*/
using UniqueAny = boost::anys::unique_any;


} // namespace josh
