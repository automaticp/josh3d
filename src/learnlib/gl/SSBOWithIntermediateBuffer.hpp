#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <type_traits>
#include <vector>
#include <ranges>


namespace learn {


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

    // Returns true if the size of the storage changed on update.
    template<typename InputIt>
    bool update(InputIt begin, InputIt end) {
        const size_t old_size = storage_.size();

        storage_.clear();
        storage_.insert(storage_.cend(), begin, end);

        const size_t new_size = storage_.size();

        const bool was_resized = old_size != new_size;

        update_ssbo_from_ready_storage(was_resized);

        return was_resized;
    }

    bool update(std::ranges::input_range auto&& range) {
        const size_t old_size = storage_.size();

        // This alternative implementation would not exist
        // if the std::distance or std::vector::insert would
        // accept iterator-sentinel pairs...

        storage_.clear();
        storage_.reserve(std::ranges::distance(range));
        for (T&& value : range) {
            storage_.emplace_back(value);
        }

        const size_t new_size = storage_.size();

        const bool was_resized = old_size != new_size;

        update_ssbo_from_ready_storage(was_resized);

        return was_resized;
    }

    const std::vector<T>& storage() const noexcept { return storage_; }

    BoundSSBO bind() {
        return ssbo_.bind_to(binding);
    }

private:
    void update_ssbo_from_ready_storage(bool needs_resizing) {
        ssbo_.bind_to(binding)
            .and_then_with_self([&, this](BoundSSBO& ssbo) {
                if (needs_resizing) {
                    ssbo.attach_data(storage_.size(), storage_.data(), usage_);
                } else [[likely]] {
                    ssbo.sub_data(storage_.size(), 0, storage_.data());
                }
            }
        );
    }
};





} // namespace learn

