#pragma once
#include <string>


namespace josh {


struct Name {
    std::string text;

    constexpr auto c_str() const noexcept -> std::string::const_pointer { return text.c_str(); }
    constexpr operator const std::string& () const noexcept { return text; }
    constexpr operator       std::string& ()       noexcept { return text; }
};


} // namespace josh
