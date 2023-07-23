#pragma once
#include "Filesystem.hpp"
#include <cstdlib>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>




namespace josh {


class TextureData {
private:
    // We have to play nice alongside stb_image
    // so we use malloc/free instead of new/delete.
    struct FreeDeleter {
        void operator()(unsigned char* data) {
            if (data) { std::free(data); }
        }
    };

    size_t width_;
    size_t height_;
    size_t n_channels_;
    std::unique_ptr<unsigned char[], FreeDeleter> data_{};

public:
    TextureData(size_t width, size_t height, size_t n_channels)
        : width_{ width }
        , height_{ height }
        , n_channels_{ n_channels }
        , data_{ (unsigned char*)std::malloc(size()) }
    {}

    static TextureData from_file(const File& file,
        bool flip_vertically = true, int num_desired_channels = 0) noexcept(false);

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    size_t height() const noexcept { return height_; }
    size_t width() const noexcept { return width_; }
    size_t n_channels() const noexcept { return n_channels_; }
    size_t n_pixels() const noexcept { return width_ * height_; }
    unsigned char* data() noexcept { return data_.get(); }
    const unsigned char* data() const noexcept { return data_.get(); }
    unsigned char& operator[](size_t idx) noexcept { return data_[idx]; }
    unsigned char operator[](size_t idx) const noexcept { return data_[idx]; }

private:
    TextureData(std::unique_ptr<unsigned char[], FreeDeleter> data, size_t width, size_t height, size_t n_channels)
        : width_{ width }
        , height_{ height }
        , n_channels_{ n_channels }
        , data_{ std::move(data) }
    {}

};



} // namespace josh
