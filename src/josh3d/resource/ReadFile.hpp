#pragma once
#include "RuntimeError.hpp"
#include "Filesystem.hpp"
#include <fstream>


namespace josh {


namespace error {

class FileReadingError final : public RuntimeError {
public:
    static constexpr auto prefix = "Cannot Read File: ";
    Path path;
    FileReadingError(Path path)
        : RuntimeError(prefix, path)
        , path{ std::move(path) }
    {}
};

} // namespace error




inline std::string read_file(const File& file) {
    std::ifstream ifs{ file.path() };
    if (ifs.fail()) {
        throw error::FileReadingError(file.path());
    }

    return std::string{
        std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()
    };
}




} // namespace josh
