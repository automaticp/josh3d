#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <cstddef>
#include <variant>
#include <utility>
#include <stb_image.h>



namespace learn {



class StbImageData {
private:
    struct StbImageDeleter {
        void operator()(unsigned char* data) {
            if (data) { stbi_image_free(reinterpret_cast<stbi_uc*>(data)); }
        }
    };

    size_t width_;
    size_t height_;
    size_t n_channels_;
    std::unique_ptr<unsigned char[], StbImageDeleter> data_{};

public:
    StbImageData(const std::string& path, bool flip_vertically = true, int num_desired_channels = 0)
        : StbImageData(path.c_str(), flip_vertically, num_desired_channels)
    {}

    StbImageData(const char* path, bool flip_vertically = true, int num_desired_channels = 0) {

        stbi_set_flip_vertically_on_load(flip_vertically);

        int width, height, n_channels;
        data_.reset(
            reinterpret_cast<unsigned char*>(
                stbi_load(path, &width, &height, &n_channels, num_desired_channels)
            )
        );
        if (!data_) {
            throw std::runtime_error(
                "Stb could not load the image at " + std::string(path)
                + ". Reason: " + stbi_failure_reason()
            );
        }

        this->width_      = width;
        this->height_     = height;
        this->n_channels_ = n_channels;
    }


    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    unsigned char* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class ImageData {
private:
    size_t width_;
    size_t height_;
    size_t n_channels_;
    std::unique_ptr<unsigned char[]> data_;

public:
    ImageData(size_t width, size_t height, size_t n_channels)
        : width_{ width }, height_{ height }, n_channels_{ n_channels },
        data_{ std::make_unique_for_overwrite<unsigned char[]>(size()) } {}

    ImageData(std::unique_ptr<unsigned char[]> data, size_t width, size_t height, size_t n_channels)
        : width_{ width }, height_{ height }, n_channels_{ n_channels }, data_{ std::move(data) } {}

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    unsigned char* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class TextureData {
public:
    using variant_t = std::variant<ImageData, StbImageData>;

private:
    variant_t variant_;

public:
    explicit(false) TextureData(variant_t image_variant) : variant_{ std::move(image_variant) } {}
    size_t size() const noexcept { return std::visit([](auto&& v) { return v.size(); }, variant_); }
    unsigned char* data() const noexcept { return std::visit([](auto&& v) { return v.data(); }, variant_); }
    size_t width() const noexcept { return std::visit([](auto&& v) { return v.width(); }, variant_); }
    size_t height() const noexcept { return std::visit([](auto&& v) { return v.height(); }, variant_); }
    size_t n_channels() const noexcept { return std::visit([](auto&& v) { return v.n_channels(); }, variant_); }

    operator variant_t() && noexcept { return std::move(variant_); }
};




} // namespace learn
