#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "CommonConcepts.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLMutability.hpp"
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "DecayToRaw.hpp" // IWYU pragma: export
#include "Region.hpp"
#include <algorithm>
#include <bit>
#include <type_traits>


namespace josh {


template<typename T>
auto allocate_buffer(
    NumElems               num_elements,
    const StoragePolicies& policies = {})
        -> UniqueBuffer<std::remove_const_t<T>>
{
    UniqueBuffer<T> buffer;
    buffer->allocate_storage(num_elements, policies);
    return buffer;
}

template<typename T>
auto specify_buffer(
    std::span<T>           src_buf,
    const StoragePolicies& policies = {})
        -> UniqueBuffer<std::remove_const_t<T>>
{
    UniqueBuffer<std::remove_const_t<T>> buffer;
    buffer->specify_storage(src_buf, policies);
    return buffer;
}

template<typename T>
auto allocate_buffer_like(RawBuffer<T, GLConst> buffer)
    -> UniqueBuffer<T>
{
    const StoragePolicies policies = buffer.get_storage_policies();
    const NumElems num_elements    = buffer.get_num_elements();
    return allocate_buffer<T>(num_elements, policies);
}

template<typename T>
auto copy_buffer(RawBuffer<T, GLConst> buffer)
    -> UniqueBuffer<T>
{
    const StoragePolicies policies = buffer.get_storage_policies();
    const NumElems num_elements    = buffer.get_num_elements();

    UniqueBuffer<T> new_buffer = allocate_buffer<T>(num_elements, policies);
    buffer.copy_data_to(new_buffer, num_elements);

    return new_buffer;
}


namespace detail {

template<typename T>
void replace_buffer_like(UniqueBuffer<T>& buffer, NumElems elem_count)
{
    const StoragePolicies policies = buffer->get_storage_policies();
    buffer = UniqueBuffer<T>();
    buffer->allocate_storage(elem_count, policies);
}

} // namespace detail


/*
Contents and buffer object "name" are invalidated on resize.
`policies` are passed into the allocation call if resize happens.

Returns true if resize occurred, false otherwise.
*/
template<trivially_copyable T>
auto resize_to_fit(
    UniqueBuffer<T>&       buffer,
    NumElems               new_elem_count,
    const StoragePolicies& policies = {})
        -> bool
{
    const NumElems old_elem_count = buffer->get_num_elements();

    if (new_elem_count != old_elem_count)
    {
        if (old_elem_count == 0)
        {
            // That means the buffer does not have a storage allocated yet.
            // Allocate one with the provided policies.
            buffer->allocate_storage(new_elem_count, policies);
        }
        else if (new_elem_count == 0)
        {
            buffer = {}; // NOTE: Policies are dropped here.
        }
        else
        {
            buffer = {};
            buffer->allocate_storage(new_elem_count, policies);
        }
        return true;
    }
    return false;
}

template<trivially_copyable T>
auto expand_to_fit(
    UniqueBuffer<T>&       buffer,
    NumElems               desired_elem_count,
    const StoragePolicies& policies = {})
        -> bool
{
    const NumElems old_elem_count = buffer->get_num_elements();

    if (desired_elem_count > old_elem_count)
    {
        if (old_elem_count != 0)
            buffer = {};

        buffer->allocate_storage(desired_elem_count, policies);
        return true;
    }
    return false;
}

/*
Returns the new number of elements.
*/
template<trivially_copyable T>
auto expand_to_fit_amortized(
    UniqueBuffer<T>&       buffer,
    NumElems               desired_elem_count,
    const StoragePolicies& policies = {},
    double                 amortization_factor = 1.5)
        -> NumElems
{
    assert(amortization_factor >= 1.0);
    const NumElems old_elem_count = buffer->get_num_elements();

    if (desired_elem_count > old_elem_count)
    {
        const NumElems amortized_count = GLsizeiptr(double(old_elem_count) * amortization_factor);

        // If the desired size is below the amortized size, then we are good
        // to allocate the amortized size.
        //
        // However, if the desired size exceeds the amortized size, then
        // we allocate exactly the desired size instead.
        const NumElems new_elem_count = std::max(amortized_count, desired_elem_count);

        if (old_elem_count != 0)
            buffer = {};

        buffer->allocate_storage(new_elem_count, policies);
        return new_elem_count;
    }
    return old_elem_count;
}


template<TextureTarget TargetV, typename ...Args>
[[nodiscard]]
auto allocate_texture(Args&&... args)
{
    // WTF?
    using UniqueTextureType = GLUnique<typename detail::texture_target_raw_mutable_type<TargetV>::type>;
    UniqueTextureType texture;
    texture->allocate_storage(FORWARD(args)...);
    return texture;
}

/*
A small utility for calculating a maximum number of mip levels for a given resolution,
such that the last level in the chain would be exactly 1x1 pixels.
*/
inline constexpr auto max_num_levels(const Size1I& resolution)
    -> NumLevels
{
    const auto width_levels = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    return NumLevels{ width_levels };
}

inline constexpr auto max_num_levels(const Size2I& resolution)
    -> NumLevels
{
    const auto width_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    const auto height_levels = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.height))));
    return NumLevels{ std::max(width_levels, height_levels) };
}

inline constexpr auto max_num_levels(const Size3I& resolution)
    -> NumLevels
{
    auto width_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    auto height_levels = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.height))));
    auto depth_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.depth))));
    return NumLevels{ std::max({ width_levels, height_levels, depth_levels }) };
}


/*
    // Overload for `Texture[1|2|3]D`, `Cubemap`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format,
        NumLevels                  num_levels = NumLevels{ 1 }) const noexcept
            requires mt::is_mutable && tt::has_lod && (!tt::is_array)
*/

/*
    // Overload for `TextureRectangle`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format) const noexcept
            requires mt::is_mutable && (!tt::has_lod) && (!tt::is_array) && (!tt::is_multisample)
*/

/*
    // Overload for `Texstd::maxture[1|2]DArray`, `CubemapArray`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        GLsizei                    num_array_elements,
        InternalFormat             internal_format,
        NumLevels                  num_levels = NumLevels{ 1 }) const noexcept
            requires mt::is_mutable && tt::has_lod && tt::is_array
*/

/*
    // Overload for `Texture2DMS`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format,
        NumSamples                 num_samples      = NumSamples{ 1 },
        SampleLocations            sample_locations = SampleLocations::NotFixed) const noexcept
            requires mt::is_mutable && tt::is_multisample && (!tt::is_array)
*/

/*
    // Overload for `Texture2DMSArray`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        GLsizei                    num_array_elements,
        InternalFormat             internal_format,
        NumSamples                 num_samples      = NumSamples{ 1 },
        SampleLocations            sample_locations = SampleLocations::NotFixed) const noexcept
            requires mt::is_mutable && tt::is_multisample && tt::is_array
*/


/*
Inserts a fence in the command queue and returns a new managed FenceSync object.
*/
[[nodiscard]]
inline auto create_fence()
    -> UniqueFenceSync
{
    return {};
}

/*
NOTE: Defaults replicate GL defaults as per the spec (Table 23.18).
*/
struct SamplerParams
{
    MinFilter      min_filter     = MinFilter::NearestMipmapLinear;
    MagFilter      mag_filter     = MagFilter::Linear;
    Wrap           wrap_s         = Wrap::Repeat;
    Wrap           wrap_t         = Wrap::Repeat;
    Wrap           wrap_r         = Wrap::Repeat;
    Optional<Wrap> wrap_all       = {}; // Will override other wrap_* params if specified.
    RGBAF          border_color   = {};
    float          min_lod        = -1000.0f;
    float          max_lod        = 1000.0f;
    float          lod_bias       = 0.0f;
    float          max_anisotropy = 1.0f;
    bool           compare_ref_depth_to_texture = false;
    CompareOp      compare_func   = CompareOp::LEqual;
};

inline auto create_sampler(const SamplerParams& params)
    -> UniqueSampler
{
    // NOTE: Trying to reduce driver chatter by only calling the api
    // when values differ from the defaults. No idea if it helps...
    constexpr SamplerParams defaults = {};
    UniqueSampler s;

#define SET_PARAM(PName)                \
    if (params.PName != defaults.PName) \
        s->set_##PName(params.PName)

    SET_PARAM(min_filter);
    SET_PARAM(mag_filter);
    if (params.wrap_all)
    {
        s->set_wrap_all(*params.wrap_all);
    }
    else
    {
        SET_PARAM(wrap_s);
        SET_PARAM(wrap_t);
        SET_PARAM(wrap_r);
    }
    if (params.border_color.r != defaults.border_color.r or
        params.border_color.g != defaults.border_color.g or
        params.border_color.b != defaults.border_color.b or
        params.border_color.a != defaults.border_color.a)
    {
        s->set_border_color_float(params.border_color);
    }
    SET_PARAM(min_lod);
    SET_PARAM(max_lod);
    SET_PARAM(lod_bias);
    SET_PARAM(max_anisotropy);
    SET_PARAM(compare_ref_depth_to_texture);
    SET_PARAM(compare_func);

#undef SET_PARAM

    return s;
}

} // namespace josh
