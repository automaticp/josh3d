#pragma once
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





} // namespace josh
