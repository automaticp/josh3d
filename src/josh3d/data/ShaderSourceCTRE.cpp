#include "ShaderSource.hpp"
#include <ctre.hpp>
#include <ctre/wrapper.hpp>
#include <ranges>


/*
This exists as a separate file because CTRE *destroys* compile times and
slows down clangd significantly when analyzing files, impacting
usability even outside the functions using CTRE.

All implementations reliant on CTRE are moved into this TU.
*/
namespace josh {


auto ShaderSource::find_version_directive(const_subrange subrange) noexcept
    -> std::optional<VersionDirective>
{
    // 1 or 2 Captures:                                      ( 1 )   (        2         )?
    // Example:                                #   version    430     core
    auto match = ctre::multiline_search<R"(^\h*#\h*version\h+(\d+)\h+(core|compatibility)?\h*$)">(subrange);

    if (match) {
        const auto& [full, version, profile] = match;
        return VersionDirective{
            // NOTE: `subrange` can be constructed from other ranges.
            .full    = full,
            .version = version,
            .profile = profile,
        };
    } else {
        return std::nullopt;
    }
}


auto ShaderSource::find_include_directive(const_subrange subrange) noexcept
    -> std::optional<IncludeDirective>
{
    // 1 Capture:                                            (    1    )
    // Example:                                #   include    "xx.glsl"
    auto match = ctre::multiline_search<R"(^\h*#\h*include\h*(<.+>|".+")\h*$)">(subrange);
    // Captures either: "..." or <...>, so need to strip chars later.

    if (match) {
        const auto& [full, quoted_path] = match;
        return IncludeDirective{
            .full        = full,
            .quoted_path = quoted_path,
            .path        = { quoted_path.begin() + 1, quoted_path.end() - 1 },
        };
    } else {
        return std::nullopt;
    }
}


auto ShaderSource::find_include_extension_directive(const_subrange subrange) noexcept
    -> std::optional<IncludeExtensionDirective>
{
    // 1 Capture:                                                                                (             1             )
    // Example:                                #   extension   GL_GOOGLE_include_directive   :            enable
    auto match = ctre::multiline_search<R"(^\h*#\h*extension\h+GL_GOOGLE_include_directive\h*:\h*(require|enable|warn|disable)\h*$)">(subrange);

    if (match) {
        const auto& [full, behavior] = match;
        return IncludeExtensionDirective{
            .full     = full,
            .behavior = behavior,
        };
    } else {
        return std::nullopt;
    }
}


} // namespace josh
