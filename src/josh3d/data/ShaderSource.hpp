#pragma once
#include <optional>
#include <string>
#include <utility>
#include <string_view>
#include <algorithm>
#include <ranges>


namespace josh {


struct VersionInfo {
    std::string::const_iterator line_begin;
    std::string::const_iterator line_end;
    std::string                 version; // 120, 330, 460, etc.
    std::string                 mode;    // (empty), core, compatibility
};


struct IncludeInfo {
    std::string::const_iterator line_begin;
    std::string::const_iterator line_end;
    std::string                 include_arg; // "path/to/somefile.glsl" or <path/to/somefile.glsl>
};




class ShaderSource {
private:
    std::string text_;

public:
    explicit ShaderSource(std::string text) : text_{ std::move(text) } {}

    operator const std::string&() const noexcept { return text_; }
    const std::string& text() const noexcept { return text_; }

    bool find_and_replace(
        std::string_view target,
        std::string_view replacement)
    {
        auto pos = text_.find(target);
        if (pos != text_.npos) {
            text_.replace(pos, target.size(), replacement);
            return true;
        }
        return false;
    }

    bool find_and_replace_until_end_of_line(
        std::string_view target,
        std::string_view replacement)
    {
        auto it = it_find(text_, target);
        if (it != text_.cend()) {
            auto it_line_end = find_line_end(it);
            text_.replace(it, it_line_end, replacement);
            return true;
        }
        return false;
    }

    bool insert_line_after(size_t line_n, std::string_view str) {
        auto line_it = find_line_n(line_n);
        if ( line_it != text_.cend() ) {
            insert_as_line_at(line_it, str);
            return true;
        }
        return false;
    }

    bool find_and_insert_as_next_line(std::string_view target, std::string_view new_line) {

        auto it = it_find(text_, target);
        if ( it == text_.cend() ) {
            return false;
        }

        it = std::find(it + target.size(), text_.cend(), '\n');

        if ( it != text_.cend() ) {
            ++it; // Move to the next line
            insert_as_line_at(it, new_line);
        } else {
            insert_as_last_line_at(it, new_line);
        }
        return true;

    }

    // No #line respecification, who cares.
    void replace_include_line_with_contents(
        std::string::const_iterator include_begin,
        std::string::const_iterator include_end,
        const ShaderSource&         contents)
    {
        // Skip #version directive, if it exists.
        if (auto ver_info = contents.find_version_directive()) {
            const auto split_contents = {
                std::string_view{ contents.text_.begin(), ver_info->line_begin },
                std::string_view{ ver_info->line_end,     contents.text_.end() }
            };

            auto versionless_view = split_contents | std::views::join;

            // Replace contents of the line, skipping #version.
            text_.replace(include_begin, include_end, versionless_view.begin(), versionless_view.end());

        } else {
            // Replace contents of the line.
            text_.replace(include_begin, include_end, contents.text());
        }

    }


    std::optional<VersionInfo> find_version_directive() const noexcept;
    std::optional<IncludeInfo> find_include_directive() const noexcept;

private:
    auto which_line(std::string::const_iterator target_it) const noexcept
        -> size_t
    {
        // Storing line lenghts in a separate array would make these searches faster, but ohwell...
        size_t line{ 0 };
        for (auto it = text_.cbegin();
            it != target_it && it != text_.end();
            ++it)
        {
            if (*it == '\n') {
                ++line;
            }
        }
        return line;
    }


    auto find_line_n(size_t line_n) const noexcept
        -> std::string::const_iterator
    {
        // Zero-based
        auto it = text_.cbegin();
        for (; line_n && (it != text_.cend()); ++it) {
            if (*it == '\n') {
                --line_n;
            }
        }
        return it;
    }

    auto find_line_end(std::string::const_iterator current_pos) const noexcept
        -> std::string::const_iterator
    {
        auto it = std::find(current_pos, text_.cend(), '\n');
        if (it != text_.cend()) {
            return std::next(it); // Point past the '\n' char.
        } else {
            return it; // EOF, so no choice.
        }
    }

    // std::string::find alternative that returns an iterator
    static auto it_find(const std::string& str, std::string_view target) noexcept
        -> std::string::const_iterator
    {
        auto pos = str.find(target);
        return (pos != str.npos) ? str.cbegin() + pos : str.cend();
    }

    // Appends a newline character and inserts into text at iterator it
    auto insert_as_line_at(std::string::const_iterator it, std::string_view new_line)
        -> std::string::const_iterator
    {
        std::string to_insert{ std::string(new_line) + '\n' };
        auto line_end_it = text_.insert(it, to_insert.begin(), to_insert.end());
        return line_end_it;
    }

    // Weird edge case where EOF is not preceded by '\n'
    auto insert_as_last_line_at(std::string::const_iterator it, std::string_view new_line)
        -> std::string::const_iterator
    {
        std::string to_insert{ '\n' + std::string(new_line) + '\n' };
        auto line_end_it = text_.insert(it, to_insert.begin(), to_insert.end());
        return line_end_it;
    }

};



} // namespace josh
