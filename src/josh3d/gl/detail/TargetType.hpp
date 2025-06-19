#pragma once
#include "EnumUtils.hpp"
#include "GLScalars.hpp"
#include <concepts>
#include <type_traits>


namespace josh::detail {


/*
Common way to infer the `target` argument for certain allocators (Texture, Query, etc).
*/
template<typename RawH>
concept specifies_target_type = requires
{
    RawH::target_type;
    requires std::same_as<std::underlying_type_t<GLenum>,
        underlying_type_or_type_t<decltype(RawH::target_type)>>;
};

/*
Provides a constexpr value identifying a target type of the handle.
Used to conditionally mixin this property into GLUnique and GLShared
provided the owned raw handle type specifies the same.
*/
template<typename RawH> struct target_type_if_specified {};

template<specifies_target_type RawH> struct target_type_if_specified<RawH>
{
    static constexpr auto target_type = RawH::target_type;
};



} // namespace josh::detail
