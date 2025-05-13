#pragma once
#include "MapMacro.hpp"
#include <concepts>
#include <type_traits>


namespace josh {


template<typename EnumT>
concept enumeration = std::is_enum_v<EnumT>;


template<typename EnumT>
concept enum_class =
    enumeration<EnumT> &&
    !std::is_convertible_v<EnumT, std::underlying_type_t<EnumT>>;



template<enumeration EnumT>
constexpr auto to_underlying(EnumT enum_value) noexcept
    -> std::underlying_type_t<EnumT>
{
    return static_cast<std::underlying_type_t<EnumT>>(enum_value);
}


template<typename T>
struct underlying_type_or_type  { using type = T; };

template<enumeration EnumT>
struct underlying_type_or_type<EnumT> { using type = std::underlying_type_t<EnumT>; };

template<typename T>
using underlying_type_or_type_t = underlying_type_or_type<T>::type;


template<typename EnumOrInt>
constexpr auto to_underlying_or_value(EnumOrInt value) noexcept
    -> underlying_type_or_type_t<EnumOrInt>
{
    return static_cast<underlying_type_or_type_t<EnumOrInt>>(value);
}



template<typename To, enumeration FromEnumT>
constexpr auto enum_cast(FromEnumT enum_value) noexcept
    -> To
        requires std::same_as<underlying_type_or_type_t<To>, std::underlying_type_t<FromEnumT>>
{
    return static_cast<To>(to_underlying(enum_value));
}

// NOLINTNEXTLINE
#define _JOSH3D_ENUM_NAME_CASE(Name) \
    case Name: return #Name;
// TODO: How do I "bind" arguments with macros?
#define JOSH3D_DEFINE_ENUM_STRING(EnumType, ...)            \
    constexpr auto enum_string(EnumType value) noexcept     \
        -> std::string_view                                 \
    {                                                       \
        switch (value) {                                    \
            using enum EnumType;                            \
            JOSH3D_MAP(_JOSH3D_ENUM_NAME_CASE, __VA_ARGS__) \
            default: return "";                             \
        }                                                   \
    }                                                       \
                                                            \
    constexpr auto enum_cstring(EnumType value) noexcept    \
        -> const char*                                      \
    {                                                       \
        switch (value) {                                    \
            using enum EnumType;                            \
            JOSH3D_MAP(_JOSH3D_ENUM_NAME_CASE, __VA_ARGS__) \
            default: return "";                             \
        }                                                   \
    }





} // namespace josh
