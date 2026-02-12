#pragma once


namespace josh::detail {


/*
Strong integer types meant to be used at the call-site to disambiguate
integer paramters of certail funtions.

Compare weak integers:
    `fbo.attach_texture_layer_to_color_buffer(tex, 3, 1, 0);`

To using strong integer types:
    `fbo.attach_texture_layer_to_color_buffer(tex, Layer{ 3 }, 1, MipLevel{ 0 });`

*/
template<typename T, typename CRTP>
struct Strong
{
    using underlying_type = T;

    T value{};

    constexpr Strong() = default;
    constexpr Strong(const T& value) : value{ value } {}

    constexpr operator T() const noexcept { return value; }

    constexpr CRTP& operator+=(const CRTP& other) noexcept { value += other.value; return self(); }
    constexpr CRTP& operator-=(const CRTP& other) noexcept { value -= other.value; return self(); }
    constexpr CRTP& operator/=(const CRTP& other) noexcept { value /= other.value; return self(); }
    constexpr CRTP& operator*=(const CRTP& other) noexcept { value *= other.value; return self(); }
    constexpr CRTP& operator++()    noexcept { ++value; return self(); }
    constexpr CRTP& operator--()    noexcept { --value; return self(); }
    constexpr CRTP  operator++(int) noexcept { return { value++ }; }
    constexpr CRTP  operator--(int) noexcept { return { value-- }; }

private:
    auto self()       noexcept ->       CRTP& { return static_cast<      CRTP&>(*this); }
    auto self() const noexcept -> const CRTP& { return static_cast<const CRTP&>(*this); }
};


} // namespace josh::detail


#define JOSH3D_DEFINE_STRONG_SCALAR(Name, UnderlyingType)                       \
    struct Name : ::josh::detail::Strong<UnderlyingType, Name> {                \
        using ::josh::detail::Strong<UnderlyingType, Name>::Strong;             \
    }



