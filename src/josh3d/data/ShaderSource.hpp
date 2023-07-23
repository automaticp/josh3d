#pragma once
#include "Filesystem.hpp"
#include <string>
#include <stdexcept>
#include <variant>
#include <utility>
#include <fstream>
#include <string_view>
#include <algorithm>


namespace josh {



inline std::string read_file(const File& file) {
    std::ifstream ifs{ file.path() };
    if (ifs.fail()) {
        // FIXME: Replace with custom error.
        throw std::runtime_error("Cannot open file: " + file.path().native());
    }

    return std::string{
        std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()
    };
}


class ShaderSource {
private:
    std::string text_;

public:
    template<typename Reader, typename... Args>
    ShaderSource(Reader&& reader, Args&&... args) : text_{ reader(std::forward<Args>(args)...) } {}

    explicit ShaderSource(std::string text) : text_{ std::move(text) } {}

    static ShaderSource from_file(const File& file) {
        return ShaderSource{ read_file(file) };
    }

    operator const std::string& () const noexcept {
        return text_;
    }

    const std::string& text() const noexcept {
        return text_;
    }

    bool find_and_replace(std::string_view target, std::string_view replacement) {
        auto pos = text_.find(target);
        if ( pos != text_.npos ) {
            text_.replace(pos, target.size(), replacement);
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


private:
    auto find_line_n(size_t line_n) const -> std::string::const_iterator {
        // Zero-based
        auto it = text_.cbegin();
        for ( ; line_n && (it != text_.cend()); ++it ) {
            if ( *it == '\n' ) {
                --line_n;
            }
        }
        return it;
    }

    // std::string::find alternative that returns an iterator
    static auto it_find(const std::string& str, std::string_view target) -> std::string::const_iterator {
        auto pos = str.find(target);
        return ( pos != str.npos ) ? str.cbegin() + pos : str.cend();
    }

    // Appends a newline character and inserts into text at iterator it
    auto insert_as_line_at(std::string::const_iterator it, std::string_view new_line) -> std::string::const_iterator {
        std::string to_insert{ std::string(new_line) + '\n' };
        auto line_end_it = text_.insert(it, to_insert.begin(), to_insert.end());
        return line_end_it;
    }

    // Weird edge case where EOF is not preceded by '\n'
    auto insert_as_last_line_at(std::string::const_iterator it, std::string_view new_line) -> std::string::const_iterator {
        std::string to_insert{ '\n' + std::string(new_line) + '\n' };
        auto line_end_it = text_.insert(it, to_insert.begin(), to_insert.end());
        return line_end_it;
    }

};



} // namespace josh
