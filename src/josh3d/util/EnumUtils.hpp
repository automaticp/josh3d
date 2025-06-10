#pragma once
#include "MapMacro.hpp"
#include <concepts>
#include <iterator>
#include <span>
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
struct underlying_type_or_type { using type = T; };

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
    requires std::same_as<underlying_type_or_type_t<To>, std::underlying_type_t<FromEnumT>>
constexpr auto enum_cast(FromEnumT enum_value) noexcept
    -> To
{
    return static_cast<To>(to_underlying(enum_value));
}

// NOLINTBEGIN(bugprone-macro-parentheses)
#define JOSH3D_DEFINE_ENUM_BITSET_OPERATORS(Enum) \
    constexpr auto  operator| (Enum lhs, Enum rhs) noexcept { return Enum(to_underlying(lhs) | to_underlying(rhs)); } \
    constexpr auto  operator& (Enum lhs, Enum rhs) noexcept { return Enum(to_underlying(lhs) & to_underlying(rhs)); } \
    constexpr auto  operator^ (Enum lhs, Enum rhs) noexcept { return Enum(to_underlying(lhs) ^ to_underlying(rhs)); } \
    constexpr auto& operator|=(Enum& lhs, Enum rhs) noexcept { return lhs = lhs | rhs; }                              \
    constexpr auto& operator&=(Enum& lhs, Enum rhs) noexcept { return lhs = lhs & rhs; }                              \
    constexpr auto& operator^=(Enum& lhs, Enum rhs) noexcept { return lhs = lhs ^ rhs; }                              \
    constexpr auto  operator~ (Enum e) noexcept { return Enum(~to_underlying(e)); }                                   \
// NOLINTEND(bugprone-macro-parentheses)

// NOLINTBEGIN(bugprone-reserved-identifier)
#define _JOSH3D_ENUM_CASE_NAME_RETURN_NAME(Name) \
    case Name: return #Name;
#define _JOSH3D_ENUM_CASE_NAME(Name) \
    case Name:
// NOLINTEND(bugprone-reserved-identifier)

// TODO: How do I "bind" arguments with macros?
#define JOSH3D_DEFINE_ENUM_EXTRAS(EnumType, ...)            \
    constexpr auto enum_string(EnumType value) noexcept     \
        -> std::string_view                                 \
    {                                                       \
        switch (value)                                      \
        {                                                   \
            using enum EnumType;                            \
            JOSH3D_MAP(_JOSH3D_ENUM_CASE_NAME_RETURN_NAME,  \
                __VA_ARGS__)                                \
            default: return "";                             \
        }                                                   \
    }                                                       \
    constexpr auto enum_cstring(EnumType value) noexcept    \
        -> const char*                                      \
    {                                                       \
        switch (value)                                      \
        {                                                   \
            using enum EnumType;                            \
            JOSH3D_MAP(_JOSH3D_ENUM_CASE_NAME_RETURN_NAME,  \
                __VA_ARGS__)                                \
            default: return "";                             \
        }                                                   \
    }                                                       \
    constexpr auto enum_valid(EnumType value) noexcept      \
        -> bool                                             \
    {                                                       \
        switch (value)                                      \
        {                                                   \
            using enum EnumType;                            \
            JOSH3D_MAP(_JOSH3D_ENUM_CASE_NAME, __VA_ARGS__) \
                return true;                                \
            default:                                        \
                return false;                               \
        }                                                   \
    }                                                       \
    namespace enum_detail::E##EnumType {                    \
    using enum EnumType;                                    \
    constexpr enum EnumType values[]{ __VA_ARGS__ };        \
    }                                                       \
    constexpr auto enum_size(EnumType) noexcept             \
        -> size_t                                           \
    {                                                       \
        return std::size(enum_detail::E##EnumType::values); \
    }                                                       \
    constexpr auto enum_iter(EnumType) noexcept             \
        -> std::span<const EnumType>                        \
    {                                                       \
        return std::span(enum_detail::E##EnumType::values); \
    }

template<enumeration E>
constexpr auto enum_size() noexcept
    -> size_t
{
    // NOTE: Just (ab)using ADL.
    return enum_size(E{});
}

template<enumeration E>
constexpr auto enum_iter() noexcept
    -> std::span<const E>
{
    return enum_iter(E{});
}


} // namespace josh
