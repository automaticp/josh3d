#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <type_traits>
#include <vector>
#include <ranges>


namespace learn {


template<typename T> requires (std::is_trivially_copyable_v<T>)
class SSBOWithIntermediateBuffer;

template<typename T>
class BoundSSBOWithIntermediateBuffer
    : public detail::AndThen<BoundSSBOWithIntermediateBuffer<T>>
{
private:
    friend class SSBOWithIntermediateBuffer<T>;

    BoundSSBO ssbo_;
    SSBOWithIntermediateBuffer<T>& parent_;

    BoundSSBOWithIntermediateBuffer(SSBOWithIntermediateBuffer<T>& parent, BoundSSBO&& ssbo)
        : parent_{ parent }
        , ssbo_{ ssbo }
    {}

public:
    // Reads the data from SSBO into the intermediate storage.
    // Effectively calls glGetBufferSubData.
    BoundSSBOWithIntermediateBuffer& read_to_storage() {
        ssbo_.get_sub_data(parent_.storage_.size(), 0, parent_.storage_.data());
        return *this;
    }

    // Allocates new storage for SSBO and intermediate buffer with specified size.
    // Use update() if you need to upload data to SSBO with automatic resizing.
    // Use create_storage() if you want to prepare SSBO for reading shader output.
    BoundSSBOWithIntermediateBuffer& create_storage(size_t new_size) {
        parent_.storage_.resize(new_size);
        ssbo_.attach_data<T>(new_size, nullptr, parent_.usage_);
        return *this;
    }

    // Updates the SSBO and intermediate storage from a pair if input iterators.
    template<typename InputIt>
    BoundSSBOWithIntermediateBuffer& update(InputIt begin, InputIt end) {
        auto& storage = parent_.storage_;

        const size_t old_size = storage.size();

        storage.clear();
        storage.insert(storage.cend(), begin, end);

        const size_t new_size = storage.size();
        const bool was_resized = old_size != new_size;

        update_ssbo_from_ready_storage(was_resized);

        return *this;
    }

    // Updates the SSBO and intermediate storage from a range.
    // Works with iterator-sentinel ranges.
    BoundSSBOWithIntermediateBuffer& update(std::ranges::input_range auto&& range) {
        auto& storage = parent_.storage_;

        const size_t old_size = storage.size();

        // This alternative implementation would not exist
        // if the std::distance or std::vector::insert would
        // accept iterator-sentinel pairs...

        storage.clear();
        storage.reserve(std::ranges::distance(range));
        for (T&& value : range) {
            storage.emplace_back(value);
        }

        const size_t new_size = storage.size();

        const bool was_resized = old_size != new_size;

        update_ssbo_from_ready_storage(was_resized);

        return *this;
    }


private:
    void update_ssbo_from_ready_storage(bool needs_resizing) {
        const auto& storage = parent_.storage_;
        if (needs_resizing) {
            ssbo_.attach_data(storage.size(), storage.data(), parent_.usage_);
        } else [[likely]] {
            ssbo_.sub_data(storage.size(), 0, storage.data());
        }
    }

};




/*
A helper used for cases where you have a non-contiguous view
over elements that need to be submitted to an SSBO, whereas that
can only be done from a contiguous storage.

Manages an internal contiguous storage that can be updated from
a view and resizes appropriately.

The assumption is that N copies plus SSBO update is faster than N draw calls
for cases where instancing is desired, or that it is your only option anyway
for cases where it's not.

Requires T to be trivially copyable for obvious reasons (SSBO does memcpy).
*/
template<typename T> requires (std::is_trivially_copyable_v<T>)
class SSBOWithIntermediateBuffer {
private:
    SSBO ssbo_;
    std::vector<T> storage_;
    gl::GLenum usage_{ gl::GL_STATIC_DRAW };

    friend class BoundSSBOWithIntermediateBuffer<T>;

public:
    gl::GLint binding;

public:
    explicit SSBOWithIntermediateBuffer(gl::GLint binding_index) noexcept
        : binding{ binding_index }
    {}

    SSBOWithIntermediateBuffer(gl::GLint binding_index, gl::GLenum usage) noexcept
        : binding{ binding_index }
        , usage_{ usage }
    {}

    size_t size() const noexcept { return storage_.size(); }
    const std::vector<T>& storage() const noexcept { return storage_; }

    BoundSSBOWithIntermediateBuffer<T> bind() {
        return { *this, ssbo_.bind_to(binding) };
    }

};





} // namespace learn

