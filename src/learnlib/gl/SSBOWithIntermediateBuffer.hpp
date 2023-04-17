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
public:
    gl::GLint binding;
    gl::GLenum usage{ gl::GL_STATIC_DRAW };

public:
    explicit SSBOWithIntermediateBuffer(gl::GLint binding_index) noexcept
        : binding{ binding_index }
    {}

    SSBOWithIntermediateBuffer(gl::GLint binding_index, gl::GLenum usage) noexcept
        : binding{ binding_index }
        , usage{ usage }
    {}


    template<typename InputIt>
    void update(InputIt begin, InputIt end) {
        const size_t old_size = storage_.size();

        storage_.clear();
        storage_.insert(storage_.cend(), begin, end);

        const size_t new_size = storage_.size();

        const bool was_resized = old_size != new_size;

        ssbo_.bind_to(binding).and_then_with_self([&, this](BoundSSBO& ssbo) {
            if (was_resized) {
                ssbo.attach_data(storage_.size(), storage_.data(), usage);
            } else [[likely]] {
                ssbo.sub_data(storage_.size(), 0, storage_.data());
            }
        });
    }

    void update(std::ranges::input_range auto&& range) {
        update(std::begin(range), std::end(range));
    }

};





} // namespace learn

