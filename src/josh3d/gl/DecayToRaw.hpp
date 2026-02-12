#pragma once
#include "detail/RawGLHandle.hpp" // IWYU pragma: keep (concepts)
#include "detail/StaticAssertFalseMacro.hpp"
#include <type_traits>


namespace josh {


/*
FIXME: This is not a well-behaved concept.
*/
template<typename T>
concept supports_get_raw_interface = requires(T owner)
{
    { owner.get() } -> detail::has_basic_raw_handle_semantics;
};

/*
Helper for places where a GL handle argument is taken as a template.

Something like:

    template<typename T> void foo(const T& handle);

will match either Raw*, or Unique*, or Shared* type equivalently, and
then if the raw handle is desired unconditionally, `decay_to_raw` can be used:

    decay_to_raw(handle).some_member_function_of_raw_handle();

*/
template<typename T>
auto decay_to_raw(T&& owned_or_raw) noexcept
{
    if constexpr (supports_get_raw_interface<std::decay_t<T>>)
        return owned_or_raw.get();
     else if constexpr (detail::has_basic_raw_handle_semantics<std::decay_t<T>>)
        return owned_or_raw;
    else
        JOSH3D_STATIC_ASSERT_FALSE_MSG(T, "Specified type cannot decay to a Raw Handle.");
}


} // namespace josh
