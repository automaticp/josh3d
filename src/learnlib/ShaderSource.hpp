#pragma once
#include <string>
#include <stdexcept>
#include <variant>
#include <utility>
#include <fstream>


namespace learn {



struct FileReader {
    std::string operator()(const std::string& path) {
        std::ifstream file{ path };
        if ( file.fail() ) {
            throw std::runtime_error("Cannot open file: " + path);
        }

        return std::string{
            std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
        };
    }
};


class ShaderSource {
private:
    std::string text_;

public:
    template<typename Reader, typename... Args>
    ShaderSource(Reader&& reader, Args&&... args) : text_{ reader(args...) } {}

    ShaderSource(std::string text) : text_{ std::move(text) } {}

    operator const std::string& () const noexcept {
        return text_;
    }

    const std::string& text() const noexcept {
        return text_;
    }
};



} // namespace learn
