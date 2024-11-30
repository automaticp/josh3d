#pragma once
#include "CommonConcepts.hpp"
#include "MallocSupport.hpp"
#include "Pixels.hpp"
#include "Region.hpp"
#include <span>




namespace josh {



template<typename PixelT>
class PixelData {
public:
    using pixel_type   = PixelT;
    using channel_type = pixel_traits<pixel_type>::channel_type;
    static constexpr size_t n_channels = pixel_traits<PixelT>::n_channels;

    // Allocate storage for image data with pixel dimensions
    // given by `image_size`.
    // The contents of the resulting image are undefined.
    PixelData(Size2S image_size)
        : data_      { malloc_unique<PixelT[]>(image_size.area()) }
        , resolution_{ image_size }
    {}

    // Take ownership of an existing pixel storage with
    // dimensions given by `image_size`.
    // The number of pixels in `data` must be `>= image_size.area()`
    PixelData(unique_malloc_ptr<PixelT[]>&& data, Size2S image_size)
        : data_      { std::move(data) }
        , resolution_{ image_size }
    {}

    // Take ownership of `channel_data` and reinterpret it as pixels.
    static PixelData from_channel_data(
        unique_malloc_ptr<channel_type[]>&& channel_data,
        const Size2S&                       resolution);

    // Copy data from `channel_data` into a new `ImageData` instance.
    static PixelData from_channel_data(
        const channel_type* channel_data,
        const Size2S&       resolution);


    // For conformance with `sized_range` and `contiguous_range`. Not recommended for other use.
    // Prefer `resolution()`, `num_channels` and `at()`.
    size_t size()       const noexcept { return num_pixels(); }

    Size2S resolution() const noexcept { return resolution_;        }
    size_t width()      const noexcept { return resolution_.width;  }
    size_t height()     const noexcept { return resolution_.height; }
    size_t num_pixels() const noexcept { return resolution_.area(); }

    size_t size_bytes() const noexcept { return num_pixels() * sizeof(PixelT); }

          pixel_type* data()        noexcept { return data_.get(); }
    const pixel_type* data()  const noexcept { return data_.get(); }
          pixel_type* begin()       noexcept { return data_.get(); }
    const pixel_type* begin() const noexcept { return data_.get(); }
          pixel_type* end()         noexcept { return begin() + num_pixels(); }
    const pixel_type* end()   const noexcept { return begin() + num_pixels(); }

    auto at(const Index2S& idx) const noexcept
        -> const pixel_type&
    {
        return data_[idx.x + idx.y * width()];
    }

    auto at(const Index2S& idx) noexcept
        -> pixel_type&
    {
        return data_[idx.x + idx.y * width()];
    }

    auto span() const noexcept
        -> std::span<const pixel_type>
    {
        return { data(), size() };
    }

    auto span() noexcept
        -> std::span<pixel_type>
    {
        return { data(), size() };
    }


private:
    unique_malloc_ptr<PixelT[]> data_;
    Size2S                      resolution_;
};




template<typename PixelT>
auto PixelData<PixelT>::from_channel_data(
    unique_malloc_ptr<channel_type[]>&& channel_data,
    const Size2S&                       resolution)
        -> PixelData<PixelT>
{
    channel_type* ptr = channel_data.release();

    // By the magic bullshit of implicit lifetimes:
    /*
    [intro.object]

    Some operations are described as implicitly creating objects
    within a specified region of storage. For each operation that is
    specified as implicitly creating objects, that operation
    implicitly creates and starts the lifetime of zero or more
    objects of implicit-lifetime types ([basic.types.general]) in its
    specified region of storage if doing so would result in the
    program having defined behavior.

    If no such set of objects would give the program defined
    behavior, the behavior of the program is undefined.

    If multiple such sets of objects would give the program defined
    behavior, it is unspecified which such set of objects is created.

    */
    // aka. "The behavior is defined only if it is defined",
    // we commit the following pile of questionable garbage:

    PixelT* px_ptr = std::launder(reinterpret_cast<PixelT*>(ptr));

    // Where `std::launder` requirements are given as:
    /*
    template< class T >
    constexpr T* launder( T* p ) noexcept;

    Formally, given

    1. the pointer p represents the address A of a byte in memory
    2. an object x is located at the address A
    3. x is within its lifetime
    4. the type of x is the same as T, ignoring cv-qualifiers
       at every level
    5. every byte that would be reachable through the result
       is reachable through p

    Then std::launder(p) returns a value of type T* that points to
    the object x. Otherwise, the behavior is undefined.
    */
    // and the contentious points (2-4) are satisfied
    // due to the implicit lifetime rules above. That is, `PixelT`
    // has it's lifetime implicitly started in `ptr`
    // because it can (???) and because that would give the program
    // defined behaviour (???).

    // If you think that I know what I'm talking about here
    // because I quote the standard, you're wrong.
    // The wording on implicit lifetimes is immensly confusing,
    // I'm bringing it up to explain my interpretation.
    //
    // Realistically, though, this is me mostly praying that
    // the compiler understands what I'm trying to do,
    // while I shove a `launder` at it's feet to make it scared of
    // touching things too much.

    return {
        unique_malloc_ptr<PixelT[]>{ px_ptr }, resolution
    };
}


template<typename PixelT>
auto PixelData<PixelT>::from_channel_data(
    const channel_type* channel_data,
    const Size2S&       image_size_in_pixels)
        -> PixelData<PixelT>
{
    PixelData<PixelT> result{ image_size_in_pixels };
    std::memcpy(result.data(), channel_data, result.size_bytes());
    return result;
}




// Helper for converting between different pixel layouts and values.
template<
    typename ResPixelT, typename InPixelT,
    of_signature<ResPixelT(const InPixelT&)> MappingF
>
auto remap_pixel_data(
    const PixelData<InPixelT>& image, MappingF&& mapping_function)
    -> PixelData<ResPixelT>
{
    PixelData<ResPixelT> result{ image.size() };
    for (size_t i{ 0 }; i < image.num_pixels(); ++i) {
        result.data()[i] =
            std::invoke(std::forward<MappingF>(mapping_function), image.data()[i]);
    }
    return result;
}




} // namespace josh
