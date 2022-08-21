#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <cstddef>
#include <stb_image.h>



namespace learn {



class StbImageData {
private:
    using stb_deleter_t = decltype(
        [](std::byte* data) {
            if (data) { stbi_image_free(reinterpret_cast<stbi_uc*>(data)); }
        }
    );
    using stb_image_owner_t = std::unique_ptr<std::byte[], stb_deleter_t>;

    size_t width_;
    size_t height_;
    size_t n_channels_;
    stb_image_owner_t data_{};

public:
    StbImageData(const std::string& path, int num_desired_channels = 0) {

        stbi_set_flip_vertically_on_load(true);

        int width, height, n_channels;
        data_.reset(
            reinterpret_cast<std::byte*>(
                stbi_load(path.c_str(), &width, &height, &n_channels, num_desired_channels)
            )
        );
        if (!data_) {
            throw std::runtime_error("Stb could not load the image at " + path);
        }

        this->width_      = width;
        this->height_     = height;
        this->n_channels_ = n_channels;
    }

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    std::byte* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class ImageData {
private:
    size_t width_;
    size_t height_;
    size_t n_channels_;
    std::unique_ptr<std::byte[]> data_;

public:
    ImageData(size_t width, size_t height, size_t n_channels)
        : width_{ width }, height_{ height }, n_channels_{ n_channels },
        data_{ std::make_unique_for_overwrite<std::byte[]>(size()) } {}

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    std::byte* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class TextureData : public std::variant<ImageData, StbImageData> {
public:
    size_t size() const noexcept { return std::visit([](auto&& v) { return v.size(); }, *this); }
    std::byte* data() const noexcept { return std::visit([](auto&& v) { return v.data(); }, *this); }
    size_t width() const noexcept { return std::visit([](auto&& v) { return v.width(); }, *this); }
    size_t height() const noexcept { return std::visit([](auto&& v) { return v.height(); }, *this); }
    size_t n_channels() const noexcept { return std::visit([](auto&& v) { return v.n_channels(); }, *this); }
};




} // namespace learn
