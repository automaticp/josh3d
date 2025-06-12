#pragma once
#include "CategoryCasts.hpp"
#include "MallocSupport.hpp"
#include "Region.hpp"
#include "Scalars.hpp"
#include <type_traits>


namespace josh {


template<typename ChannelT>
class ImageView
{
public:
    using channel_type = ChannelT; // NOTE: Could be const T.
    using value_type   = std::remove_cv_t<channel_type>;

    ImageView(
        channel_type*   ptr,
        const Extent2S& resolution,
        usize           num_channels);

    auto resolution()   const noexcept -> Extent2S { return resolution_; }
    auto resolutioni()  const noexcept -> Extent2I { return Extent2I(resolution_); }
    auto num_pixels()   const noexcept -> usize    { return resolution().area();   }
    auto num_channels() const noexcept -> usize    { return num_channels_;         }
    auto size_bytes()   const noexcept -> usize    { return sizeof(channel_type) * size(); }

    auto at(const Index2S& idx, uindex channel) const noexcept -> channel_type&;

    // The following is for conformance with `sized_range` and `contiguous_range`.
    // Not recommended for other use. Prefer `resolution()`, `num_channels()` and `at()`.

    auto size()  const noexcept -> usize         { return num_pixels() * num_channels(); }
    auto data()  const noexcept -> channel_type* { return ptr_; }
    auto begin() const noexcept -> channel_type* { return data(); }
    auto end()   const noexcept -> channel_type* { return begin() + size(); }

private:
    channel_type* ptr_;
    Extent2S      resolution_;
    usize         num_channels_;
};


template<typename ChannelT>
class ImageData
{
public:
    using channel_type = ChannelT;
    using value_type   = channel_type;

    // Allocate storage for image data with the specified resolution and
    // number of channels. The contents of the resulting image are undefined.
    ImageData(
        const Extent2S& resolution,
        usize           num_channels);

    auto view()       noexcept -> ImageView<      channel_type> { return { data(), resolution(), num_channels() }; }
    auto view() const noexcept -> ImageView<const channel_type> { return { data(), resolution(), num_channels() }; }
    operator ImageView<channel_type>()            & noexcept { return view(); }
    operator ImageView<const channel_type>() const& noexcept { return view(); }

    auto resolution()   const noexcept -> Extent2S { return resolution_; }
    auto resolutioni()  const noexcept -> Extent2I { return Extent2I(resolution_); }
    auto num_pixels()   const noexcept -> usize    { return resolution().area();   }
    auto num_channels() const noexcept -> usize    { return num_channels_;         }
    auto size_bytes()   const noexcept -> usize    { return sizeof(channel_type) * size(); }

    auto at(const Index2S& idx, uindex channel)       noexcept ->       channel_type&;
    auto at(const Index2S& idx, uindex channel) const noexcept -> const channel_type&;

    // The following is for conformance with `sized_range` and `contiguous_range`.
    // Not recommended for other use. Prefer `resolution()`, `num_channels()` and `at()`.

    auto size()  const noexcept -> usize { return num_pixels() * num_channels(); }
    auto data()        noexcept ->       channel_type* { return data_.get(); }
    auto data()  const noexcept -> const channel_type* { return data_.get(); }
    auto begin()       noexcept ->       channel_type* { return data(); }
    auto begin() const noexcept -> const channel_type* { return data(); }
    auto end()         noexcept ->       channel_type* { return begin() + size(); }
    auto end()   const noexcept -> const channel_type* { return begin() + size(); }

    // Take ownership of an existing pixel storage with the specified
    // resolution and number of channels. Unsafe.
    static auto take_ownership(
        unique_malloc_ptr<channel_type[]> data,
        const Extent2S&                   resolution,
        usize                             num_channels)
            -> ImageData;

    // Release ownership of the underlying buffer. This will reset
    // the resolution and num_channels, so query them beforehand.
    [[nodiscard]] auto release() noexcept
        -> unique_malloc_ptr<channel_type[]>;

private:
    ImageData(
        unique_malloc_ptr<ChannelT[]> data,
        const Extent2S&               resolution,
        usize                         num_channels);

    // NOTE: We rely on malloc_ptr because that's all C APIs can spit out.
    unique_malloc_ptr<channel_type[]> data_;
    Extent2S                          resolution_;
    usize                             num_channels_;
};




template<typename ChannelT>
ImageView<ChannelT>::ImageView(
    channel_type*   ptr,
    const Extent2S& resolution,
    usize           num_channels
)
    : ptr_         { ptr          }
    , resolution_  { resolution   }
    , num_channels_{ num_channels }
{}

template<typename ChannelT>
auto ImageView<ChannelT>::at(const Index2S& idx, uindex channel) const noexcept
    -> channel_type&
{
    const uindex px_offset = idx.x + idx.y * resolution_.width;
    assert(px_offset < num_pixels());
    assert(channel < num_channels());
    return ptr_[px_offset + channel];
}

template<typename ChannelT>
ImageData<ChannelT>::ImageData(
    const Extent2S& resolution,
    usize           num_channels
)
    : data_        { malloc_unique<channel_type[]>(resolution.area() * num_channels) }
    , resolution_  { resolution   }
    , num_channels_{ num_channels }
{}

template<typename ChannelT>
ImageData<ChannelT>::ImageData(
    unique_malloc_ptr<ChannelT[]> data,
    const Extent2S&               resolution,
    usize                         num_channels
)
    : data_        { MOVE(data)   }
    , resolution_  { resolution   }
    , num_channels_{ num_channels }
{}

template<typename ChannelT>
auto ImageData<ChannelT>::at(const Index2S& idx, uindex channel) noexcept
    -> channel_type&
{
    const uindex px_offset = idx.x + idx.y * resolution_.width;
    assert(px_offset < num_pixels());
    assert(channel < num_channels());
    return data_[px_offset + channel];
}

template<typename ChannelT>
auto ImageData<ChannelT>::at(const Index2S& idx, uindex channel) const noexcept
    -> const channel_type&
{
    const uindex px_offset = idx.x + idx.y * resolution_.width;
    assert(px_offset < num_pixels());
    assert(channel < num_channels());
    return data_[px_offset + channel];
}

template<typename ChannelT>
auto ImageData<ChannelT>::take_ownership(
    unique_malloc_ptr<channel_type[]> data,
    const Extent2S&                   resolution,
    usize                             num_channels)
        -> ImageData
{
    return { MOVE(data), resolution, num_channels };
}

template<typename ChannelT>
auto ImageData<ChannelT>::release() noexcept
    -> unique_malloc_ptr<channel_type[]>
{
    resolution_   = { 0, 0 };
    num_channels_ = { 0 };
    return MOVE(data_);
}


} // namespace josh
