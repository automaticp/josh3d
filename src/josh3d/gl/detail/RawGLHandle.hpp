#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include <utility>



namespace josh {
namespace detail {


/*
The base class of all OpenGL handles. Represents a fully opaque
handle that has no knowledge of its type or allocation method.

Models a raw `void*` or `const void*` depending on mutability.
*/
template<mutability_tag MutT>
class RawGLHandle : public MutT {
protected:
    // Maybe I could make something like: enum class GLHandle : GLuint {}; ?
    GLuint id_; // Should this be private?
    GLuint reset_id(GLuint new_id = GLuint{ 0 }) noexcept {
        GLuint old_id{ id_ };
        id_ = new_id;
        return old_id;
    }
    ~RawGLHandle() = default;
public:
    friend RawGLHandle<OppositeGLMutability<MutT>>;

    explicit RawGLHandle(GLuint id) : id_{ id } {}

    RawGLHandle(const RawGLHandle&)     = default;
    RawGLHandle(RawGLHandle&&) noexcept = default;
    RawGLHandle& operator=(const RawGLHandle&)     = default;
    RawGLHandle& operator=(RawGLHandle&&) noexcept = default;

    // GLMutable -> GLConst   conversions are permitted

    RawGLHandle(const RawGLHandle<GLMutable>& other) noexcept
        requires gl_const<MutT>
        : id_{ other.id_ }
    {}

    RawGLHandle& operator=(const RawGLHandle<GLMutable>& other) noexcept
        requires gl_const<MutT>
    {
        id_ = other.id_;
        return *this;
    }

    RawGLHandle(RawGLHandle<GLMutable>&& other) noexcept
        requires gl_const<MutT>
        : id_{ other.id_ }
    {}

    RawGLHandle& operator=(RawGLHandle<GLMutable>&& other) noexcept
        requires gl_const<MutT>
    {
        id_ = other.id_;
        return *this;
    }

    // GLConst   -> GLMutable conversions are forbidden

    RawGLHandle(const RawGLHandle<GLConst>&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle& operator=(const RawGLHandle<GLConst>&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle(RawGLHandle<GLConst>&&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle& operator=(RawGLHandle<GLConst>&&) noexcept
        requires gl_mutable<MutT> = delete;


    GLuint id() const noexcept { return id_; }
    explicit operator GLuint() const noexcept { return id_; }
};




// Since constructibility might not be inherited,
// it makes sense to check this for both kind-handles and object-handles,
// even if they derive from the RawGLHandle.
template<typename RawT>
concept has_basic_raw_handle_semantics = requires(RawT raw) {
    // Constructible from GLuint.
    RawT{ GLuint{ 42 } };
    // Can return or be converted to the object id.
    { raw.id() }                 -> same_as_remove_cvref<GLuint>;
    { std::as_const(raw).id() }  -> same_as_remove_cvref<GLuint>;
    { static_cast<GLuint>(raw) } -> same_as_remove_cvref<GLuint>;
};






} // namespace detail
} // namespace josh
