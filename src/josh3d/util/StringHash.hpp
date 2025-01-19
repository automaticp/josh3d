#pragma once
#include <string>
#include <string_view>


namespace josh {


struct string_hash {
    using is_transparent = void;

    auto operator()(std::string_view sv) const noexcept
        -> size_t
    {
        return std::hash<std::string_view>{}(sv);
    }

    auto operator()(const std::string& s) const noexcept
        -> size_t
    {
        return std::hash<std::string>{}(s);
    }

};



} // namespace josh
