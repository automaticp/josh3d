#pragma once
#include "CommonConcepts.hpp"
#include "GLAPI.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "Size.hpp"
#include <bit>
#include <glbinding/gl/types.h>


namespace josh::dsa {


template<typename T>
UniqueBuffer<T> allocate_buffer(
    NumElems             num_elements,
    StorageMode          storage_mode       = StorageMode::DynamicServer,
    PermittedMapping     mapping_policy     = PermittedMapping::ReadWrite,
    PermittedPersistence persistence_policy = PermittedPersistence::NotPersistent) noexcept
{
    UniqueBuffer<T> buffer;
    buffer.allocate_storage(num_elements, storage_mode, mapping_policy, persistence_policy);
    return buffer;
}


template<typename T>
UniqueBuffer<T> specify_buffer(
    std::span<const T>   src_buf,
    StorageMode          storage_mode       = StorageMode::DynamicServer,
    PermittedMapping     mapping_policy     = PermittedMapping::ReadWrite,
    PermittedPersistence persistence_policy = PermittedPersistence::NotPersistent) noexcept
{
    UniqueBuffer<T> buffer;
    buffer.specify_storage(src_buf, storage_mode, mapping_policy, persistence_policy);
    return buffer;
}




template<typename T>
UniqueBuffer<T> allocate_buffer_like(RawBuffer<T, GLConst> buffer) noexcept {
    const auto [mode, mapping, persistence] = buffer.get_storage_policies();
    const NumElems num_elements             = buffer.get_num_elements();

    return allocate_buffer<T>(num_elements, mode, mapping, persistence);
}


template<typename T>
UniqueBuffer<T> copy_buffer(RawBuffer<T, GLConst> buffer) noexcept {
    const auto [mode, mapping, persistence] = buffer.get_storage_policies();
    const NumElems num_elements             = buffer.get_num_elements();

    UniqueBuffer<T> new_buffer = allocate_buffer<T>(num_elements, mode, mapping, persistence);
    buffer.copy_data_to(new_buffer, num_elements);

    return new_buffer;
}




namespace detail {

template<typename T>
void replace_buffer_like(UniqueBuffer<T>& buffer, NumElems elem_count) noexcept {
    StoragePolicies policies = buffer.get_storage_policies();
    buffer = UniqueBuffer<T>();
    buffer.allocate_storage(elem_count, policies.storage_mode, policies.permitted_mapping, policies.permitted_persistence);
}

} // namespace detail


// Contents and buffer object "name" are invalidated on resize.
template<trivially_copyable T>
void resize_to_fit(
    UniqueBuffer<T>& buffer,
    NumElems         new_elem_count) noexcept
{
    NumElems old_elem_count = buffer.get_num_elements();

    if (old_elem_count == 0) {
        // That means the buffer does not have a storage allocated yet.
        // Allocate one with default parameters for storage mode and mapping.
        // Those are DynamicServer storage, ReadWrite permitted mapping and NotPersistent persistence.
        // If you cared enough about these flags, then you wouldn't use this function.
        buffer.allocate_storage(new_elem_count);
    } else if (new_elem_count != old_elem_count) {
        detail::replace_buffer_like(buffer, new_elem_count);
    }
}


template<trivially_copyable T>
void expand_to_fit(
    UniqueBuffer<T>& buffer,
    NumElems         desired_elem_count) noexcept
{
    NumElems old_elem_count = buffer.get_num_elements();

    if (old_elem_count == 0) {
        buffer.allocate_storage(desired_elem_count);
    } else if (desired_elem_count > old_elem_count) {
        detail::replace_buffer_like(buffer, desired_elem_count);
    }
}


template<trivially_copyable T>
void expand_to_fit_amortized(
    UniqueBuffer<T>& buffer,
    NumElems         desired_elem_count,
    double           amortization_factor = 1.5) noexcept
{
    assert(amortization_factor >= 1.0);
    NumElems old_elem_count = buffer.get_num_elements();

    if (old_elem_count == 0) {
        buffer.allocate_storage(desired_elem_count);
    } else if (desired_elem_count > old_elem_count) {
        NumElems amortized_size{
            GLsizeiptr(double(old_elem_count.value) * amortization_factor)
        };
        // If the desired size is below the amortized size, then we are good.
        // However, if the desired size exceeds the amortized size, then
        // we allocate exactly the desired size instead.
        if (desired_elem_count <= amortized_size) {
            detail::replace_buffer_like(buffer, amortized_size);
        } else /* desired exceeds amortized */ {
            detail::replace_buffer_like(buffer, desired_elem_count);
        }
    }
}





template<TextureTarget TargetV, typename ...Args>
auto allocate_texture(Args&&... args) noexcept {
    using UniqueTextureType = GLUnique<typename detail::texture_target_raw_mutable_type<TargetV>::type>;
    UniqueTextureType texture;
    texture.allocate_storage(std::forward<Args>(args)...);
    return texture;
}


// A small utility for calculating a maximum number of mip levels for a given resolution,
// such that the last level in the chain would be exactly 1x1 pixels.
inline constexpr NumLevels max_num_levels(const Size1I& resolution) noexcept {
    auto width_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    return NumLevels{ width_levels };
}

inline constexpr NumLevels max_num_levels(const Size2I& resolution) noexcept {
    auto width_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    auto height_levels = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.height))));
    return NumLevels{ std::max(width_levels, height_levels) };
}

inline constexpr NumLevels max_num_levels(const Size3I& resolution) noexcept {
    auto width_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.width))));
    auto height_levels = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.height))));
    auto depth_levels  = GLsizei(1u + std::countr_zero(std::bit_floor(GLuint(resolution.depth))));
    return NumLevels{ std::max(std::max(width_levels, height_levels), depth_levels) };
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
    // Overload for `Texture[1|2]DArray`, `CubemapArray`.
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


// This is meaningless, what you want is attachments instead.

// template<template <typename> typename RawTextureT>
//     requires of_kind<RawTextureT<GLMutable>, GLKind::Texture>
// void resize_to_fit(
//     GLUnique<RawTextureT<GLMutable>>& texture,
//     const typename texture_traits<RawTextureT<GLMutable>>::resolution_type&
//                                       resolution,
//     NumLevels                         num_levels = NumLevels{ 0 }) noexcept
//         requires
//             texture_traits<RawTextureT<GLMutable>>::has_lod
// {
//     using tt = texture_traits<RawTextureT<GLMutable>>;


// }





} // namespace josh
