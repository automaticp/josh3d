#pragma once
#include "Filesystem.hpp"
#include "RuntimeError.hpp"
#include "Size.hpp"
#include <cstdlib>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>




namespace josh {


namespace error {

class ImageReadingError final : public RuntimeError {
public:
    static constexpr auto prefix = "Cannot Read Image: ";
    Path path;
    ImageReadingError(Path path, std::string reason)
        : RuntimeError(prefix,
            path.native() + "; Reason: " + std::move(reason))
        , path{ std::move(path) }
    {}
};

} // namespace error





class TextureData {
private:
    // We have to play nice alongside stb_image
    // so we use malloc/free instead of new/delete.
    struct FreeDeleter {
        void operator()(unsigned char* data) {
            if (data) { std::free(data); }
        }
    };

    Size2S size_;
    size_t n_channels_;
    std::unique_ptr<unsigned char[], FreeDeleter> data_{};

public:
    TextureData(Size2S image_size, size_t n_channels)
        : size_{ image_size }
        , n_channels_{ n_channels }
        , data_{ (unsigned char*)std::malloc(data_size()) }
    {}

    static TextureData from_file(const File& file,
        bool flip_vertically = true, int num_desired_channels = 0) noexcept(false);

    size_t data_size() const noexcept { return size_.area() * n_channels_; }
    Size2S image_size() const noexcept { return size_; }
    size_t height() const noexcept { return size_.height; }
    size_t width() const noexcept { return size_.width; }
    size_t n_channels() const noexcept { return n_channels_; }
    size_t n_pixels() const noexcept { return size_.area(); }
    unsigned char* data() noexcept { return data_.get(); }
    const unsigned char* data() const noexcept { return data_.get(); }
    unsigned char& operator[](size_t idx) noexcept { return data_[idx]; }
    unsigned char operator[](size_t idx) const noexcept { return data_[idx]; }

private:
    TextureData(std::unique_ptr<unsigned char[], FreeDeleter> data,
        Size2S image_size, size_t n_channels)
        : size_{ image_size }
        , n_channels_{ n_channels }
        , data_{ std::move(data) }
    {}

};



} // namespace josh
