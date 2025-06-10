#pragma once
#include "MallocSupport.hpp"
#include "Region.hpp"
#include <span>
#include <utility>


namespace josh {


template<typename ChannelT>
class ImageData {
public:
    using channel_type = ChannelT;

    // Allocate storage for image data with the specified resolution
    // and number of channels. The contents of the resulting image
    // are undefined.
    ImageData(
        const Size2S& resolution,
        size_t        num_channels);

    // Take ownership of an existing pixel storage with the specified
    // resolution and number of channels.
    ImageData(
        unique_malloc_ptr<ChannelT[]>&& data,
        const Size2S&                   resolution,
        size_t                          num_channels);


    Size2S resolution()   const noexcept { return resolution_;   }
    Size2I resolutioni()  const noexcept { return Size2I(resolution_); }
    size_t num_channels() const noexcept { return num_channels_; }
    size_t size_bytes()   const noexcept {
        return sizeof(channel_type) * resolution_.area() * num_channels_;
    }

    // For conformance with `sized_range` and `contiguous_range`. Not recommended for other use.
    // Prefer `resolution()`, `num_channels()` and `at()`.
    size_t size() const noexcept { return resolution_.area() * num_channels_; }

          channel_type* data()        noexcept { return data_.get(); }
    const channel_type* data()  const noexcept { return data_.get(); }
          channel_type* begin()       noexcept { return data_.get(); }
    const channel_type* begin() const noexcept { return data_.get(); }
          channel_type* end()         noexcept { return begin() + resolution_.area() * num_channels_; }
    const channel_type* end()   const noexcept { return begin() + resolution_.area() * num_channels_; }

    auto at(const Index2S& idx, size_t channel) noexcept
        -> channel_type&
    {
        return data_[(idx.x + idx.y * resolution_.width) + channel];
    }

    auto at(const Index2S& idx, size_t channel) const noexcept
        -> const channel_type&
    {
        return data_[(idx.x + idx.y * resolution_.width) + channel];
    }

    auto span() const noexcept
        -> std::span<const channel_type>
    {
        return { data(), size() };
    }

    auto span() noexcept
        -> std::span<channel_type>
    {
        return { data(), size() };
    }

    // Release ownership of the underlying buffer.
    [[nodiscard]]
    auto release() noexcept
        -> unique_malloc_ptr<channel_type[]>
    {
        resolution_   = { 0, 0 };
        num_channels_ = { 0 };
        return std::move(data_);
    }


private:
    unique_malloc_ptr<channel_type[]> data_;
    Size2S                            resolution_;
    size_t                            num_channels_;
};




template<typename ChannelT>
ImageData<ChannelT>::ImageData(
    const Size2S& resolution,
    size_t        num_channels)
    : data_        { malloc_unique<channel_type[]>(resolution.area() * num_channels) }
    , resolution_  { resolution   }
    , num_channels_{ num_channels }
{}


template<typename ChannelT>
ImageData<ChannelT>::ImageData(
    unique_malloc_ptr<ChannelT[]>&& data,
    const Size2S&                   resolution,
    size_t                          num_channels)
    : data_        { std::move(data) }
    , resolution_  { resolution      }
    , num_channels_{ num_channels    }
{}


} // namespace josh
