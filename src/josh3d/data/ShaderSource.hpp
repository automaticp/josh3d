#pragma once
#include <optional>
#include <string>
#include <utility>
#include <string_view>
#include <ranges>


namespace josh {


/*
A constrained wrapper around a string, that enables operations
that are specific to preprocessing shader source files.
*/
class ShaderSource {
public:
    explicit ShaderSource(std::string text) : text_{ std::move(text) } {}

    using const_iterator = std::string::const_iterator;

    /*
    I can't believe *I* now have to explain why this needs to be done,
    and why I can't just return std::string_view. What a joke.

    "Yaaah, why standardize the exact types of the iterators?
    We'll leave the decision to the library implementors.
    So that they could make a mess."

    This is like a std::string_view, except it actually returns iterators
    compatible with std::string. This allows you to use it in operations
    on the underlying string of ShaderSource.
    */
    struct const_subrange : std::ranges::subrange<const_iterator> {
        using std::ranges::subrange<const_iterator>::subrange;
        auto view()      const noexcept -> std::string_view { return { begin(), end() }; }
        auto to_string() const          -> std::string      { return { begin(), end() }; }
        operator std::string_view() const noexcept { return view(); }
    };

    auto begin() const noexcept -> const_iterator { return text_.begin(); }
    auto end()   const noexcept -> const_iterator { return text_.end();   }
    auto data()  const noexcept -> const char*    { return text_.data();  }
    auto c_str() const noexcept -> const char*    { return text_.c_str(); }
    auto size()  const noexcept -> size_t         { return text_.size();  }

    // Returns a (sub)range covering the whole source text.
    auto text_range() const noexcept -> const_subrange   { return { begin(), end() }; }
    auto text_view()  const noexcept -> std::string_view { return text_;              }

    operator const_subrange()   const noexcept { return text_range(); }
    operator std::string_view() const noexcept { return text_view();  }


    struct VersionDirective {
        const_subrange full;    // Full match, without the newline (ex. "# version   430    core   ").
        const_subrange version; // "120", "330", "460", etc.
        const_subrange profile; // (empty), "core", "compatibility".
    };

    struct IncludeDirective {
        const_subrange full;        // Full match, without the newline (ex. "# include  <path/to/somefile.glsl> ").
        const_subrange quoted_path; // ""path/to/somefile.glsl"" or "<path/to/somefile.glsl>"
        const_subrange path;        // "path/to/somefile.glsl"
    };

    struct IncludeExtensionDirective {
        const_subrange full;     // Full match, excluding the newline (ex. "# extension GL_GOOGLE_include_directive :   enabled  ")
        const_subrange behavior; // "require", "enable", "warn", "disable".
    };

    [[nodiscard]] static auto find_version_directive          (const_subrange subrange) noexcept -> std::optional<VersionDirective>;
    [[nodiscard]] static auto find_include_directive          (const_subrange subrange) noexcept -> std::optional<IncludeDirective>;
    [[nodiscard]] static auto find_include_extension_directive(const_subrange subrange) noexcept -> std::optional<IncludeExtensionDirective>;

    // Replace a source `subrange` with `contents`.
    // Views and iterators may be invalidated.
    // Returns a view of the replaced region.
    auto replace_subrange(const_subrange subrange, std::string_view contents) -> const_subrange;

    // Removes the `subrange` from the source string.
    // Views and iterators may be invalidated.
    // Returns an iterator to where the subrange would begin before removal.
    auto remove_subrange(const_subrange subrange) -> const_iterator;

    // Inserts `contents` before the `pos`. This is dumb and will not consider newlines/EOF.
    // Views and iterators may be invalidated.
    // Returns a view to the inserted region.
    auto insert_before(const_iterator pos, std::string_view contents) -> const_subrange;

    // Inserts `contents` after the `pos`. This is dumb and will not consider newlines/EOF.
    // Views and iterators may be invalidated.
    // Returns a view to the inserted region.
    auto insert_after(const_iterator pos, std::string_view contents) -> const_subrange;

    // Inserts a line of `contents` on the line before line where `pos` is located.
    // Appends newline to the `contents` before insertion.
    // If `pos` points to a newline, finds another newline before `pos` and inserts after that.
    // If no newlines preceed `pos`, inserts at the beginning of the file.
    // Views and iterators may be invalidated.
    // Returns a view to the inserted region.
    auto insert_line_on_line_before(const_iterator pos, std::string_view contents) -> const_subrange;

    // Inserts a line of `contents` on the line after line where `pos` is located.
    // Appends newline to the `contents` before insertion.
    // If `pos` points to a newline, insertes right after that newline.
    // If `pos` points to the EOF, replaces it with a newline and inserts after that.
    // Views and iterators may be invalidated.
    // Returns a view to the inserted region.
    auto insert_line_on_line_after(const_iterator pos, std::string_view contents) -> const_subrange;


private:
    std::string text_;
};


} // namespace josh
