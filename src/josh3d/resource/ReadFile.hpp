#pragma once
#include "RuntimeError.hpp"
#include "Filesystem.hpp"


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
using error::FileReadingError;


auto read_file(const File& file)
    -> std::string;


} // namespace josh
