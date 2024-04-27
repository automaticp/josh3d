#include "ShaderSource.hpp"
#include <ctre.hpp>
#include <ctre/wrapper.hpp>


namespace josh {


auto ShaderSource::find_version_directive() const noexcept
    -> std::optional<VersionInfo>
{
    // 1 or 2 captures:                        (c 1)   (capture 2 optional)
    auto m  = ctre::multiline_search<R"(^\h*#\h*version\h+(\d+)\h+(core|compatibility)?\h*$)">(text_);

    if (m) {
        const auto& full = m.get<0>();
        return VersionInfo{
            .line_begin = full.begin(),
            .line_end   = full.end(), // TODO: Is \n included?
            .version    = m.get<1>().to_string(),
            .mode       = m.get<2>().to_string()
        };
    } else {
        return std::nullopt;
    }
}


auto ShaderSource::find_include_directive() const noexcept
    -> std::optional<IncludeInfo>
{
    // 1 capture:                             (capture 1)
    auto m = ctre::multiline_search<R"(^\h*#\h*include\h*(<.+>|".+")\h*$)">(text_);
    // Captures either: "..." or <...>, so need to strip chars later.

    if (m) {
        const auto& full = m.get<0>();
        return IncludeInfo {
            .line_begin  = full.begin(),
            .line_end    = full.end(),
            .include_arg = m.get<1>().to_string()
        };
    } else {
        return std::nullopt;
    }
}




} // namespace josh
