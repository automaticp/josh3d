#pragma once
#include "CommonConcepts.hpp"
#include <compare>
#include <filesystem>
#include <optional>
#include <utility>
#include <stdexcept>


namespace josh {



namespace error {

class FilesystemError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// FIXME: Actually store reason in an error string
class DirectoryDoesNotExist : public FilesystemError {
public:
    using FilesystemError::FilesystemError;
};

class NotADirectory : public FilesystemError {
public:
    using FilesystemError::FilesystemError;
};


class FileDoesNotExist : public FilesystemError {
public:
    using FilesystemError::FilesystemError;
};

class NotAFile : public FilesystemError {
public:
    using FilesystemError::FilesystemError;
};



} // namespace error




using Path = std::filesystem::path;


/*
Directory and File classes are wrappers around std::filesystem::directory_entry
that validate the entry to be either a directory or a file **at construction time**.

Due to the asynchronous nature of the filesystem,
there's no guarantee that the Directory and File objects represent
actual directories and files after construction. This approach is still
vulnerable to TOCTOU failures, which, given another layer of validation
should hopefully not result in actual bugs.

The purpose of these wrappers is primarily to "fail as early as possible"
in order to preserve more context of the failure.

For example, assume that we want to read a file:

std::string read_file(const File& file) {

    std::ifstream ifs{ file.path() };

    if (!ifs) throw error::FileReadingFailure("Cannot read file: " + file.path());

    std::string contents = ...
    return contents;
}

The ifstream failure is reported by throwing FileReadingFailure which lacks
any specific context. A very common cause for file reading failure is the file
just not existing, which we can validate in the constructor of File and throw
some FileDoesNotExist, giving the exact reason and preempting FileReadingFailure.

Note that between the construction of File and opening the file
within the ifstream constructor the file could have been erased
and become inaccessible. You SHOULD still check the ifstream for failure
because of this, else the TOCTOU condition becomes an actual bug.
*/
class File {
private:
    std::filesystem::directory_entry file_;

    struct UncheckedConstructorKey {};
    File(const Path& path, UncheckedConstructorKey)
        : file_{ path }
    {}

public:
    explicit File(const Path& path)
        : file_{ path }
    {
        if (!file_.exists()) {
            throw error::FileDoesNotExist(file_.path());
        }

        if (!file_.is_regular_file()) {
            throw error::NotAFile(file_.path());
        }
    }

    // Non-throwing static constructor.
    // Does not report the type of failure because I'm lazy to
    // include a third party `expected` implementation.
    static auto try_make(const Path& path)
        -> std::optional<File>
    {
        // FIXME: Is directory_entry constructor potentially expensive?
        // Should this be the other way around, where the check
        // comes first and the construction of Directory instance after?
        File file{ path, UncheckedConstructorKey() };

        if (file.is_valid()) {
            return { std::move(file) };
        } else {
            return std::nullopt;
        }
    }


    const std::filesystem::path& path() const noexcept {
        return file_.path();
    }

    // Vulnerable to TOCTOU, a hint, but no guarantees.
    bool is_valid() const {
        return file_.exists() && file_.is_regular_file();
    }

    bool operator==(const File&) const noexcept = default;
    std::strong_ordering operator<=>(const File&) const noexcept = default;
};




class Directory {
private:
    std::filesystem::directory_entry directory_;

    struct UncheckedConstructorKey {};
    Directory(const Path& path, UncheckedConstructorKey)
        : directory_{ path }
    {}

public:
    explicit Directory(const Path& path)
        : directory_{ path }
    {
        if (!directory_.exists()) {
            throw error::DirectoryDoesNotExist(directory_.path());
        }

        if (!directory_.is_directory()) {
            throw error::NotADirectory(directory_.path());
        }
    }

    // Non-throwing static constructor.
    static auto try_make(const Path& path)
        -> std::optional<Directory>
    {
        Directory dir{ path, UncheckedConstructorKey() };

        if (dir.is_valid()) {
            return { std::move(dir) };
        } else {
            return std::nullopt;
        }
    }


    const std::filesystem::path& path() const noexcept {
        return directory_.path();
    }

    // Vulnerable to TOCTOU, a hint, but no guarantees.
    bool is_valid() const {
        return directory_.exists() && directory_.is_directory();
    }

    bool operator==(const Directory&) const noexcept = default;
    std::strong_ordering operator<=>(const Directory&) const noexcept = default;
};




namespace filesystem_literals {

inline std::filesystem::path operator""_path(const char* str, size_t size) {
    return { str, str + size };
}

inline Directory operator""_directory(const char* str, size_t size) {
    return Directory{ std::filesystem::path{ str, str + size } };
}

inline File operator""_file(const char* str, size_t size) {
    return File{ std::filesystem::path{ str, str + size } };
}


} // namespace filesystem_literals


} // namespace josh



template<>
struct std::hash<josh::Directory> {
    std::size_t operator()(const josh::Directory& dir) const noexcept {
        return std::hash<josh::Path>{}(dir.path());
    }
};


template<>
struct std::hash<josh::File> {
    std::size_t operator()(const josh::File& file) const noexcept {
        return std::hash<josh::Path>{}(file.path());
    }
};


