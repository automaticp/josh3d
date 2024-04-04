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
template<mutability_tag MutT, typename IdType = GLuint>
class RawGLHandle {
public:
    using id_type = IdType;
private:
    id_type id_;
    friend RawGLHandle<GLConst>;
protected:
    ~RawGLHandle() = default;
public:

    explicit RawGLHandle(id_type id) : id_{ id } {}

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

    // Returns the underlying OpenGL ID (aka. Name) of the object.
    id_type id() const noexcept { return id_; }
    explicit operator id_type() const noexcept { return id_; }
};



// Use this to indicate that a type is a Raw Handle type.
// TODO: Incomplete and shaky, might be worth rethinking.
template<typename RawHandleT>
concept has_basic_raw_handle_semantics = requires(std::remove_cvref_t<RawHandleT> raw) {
    // Can return or be converted to the object id.
    { raw.id() }                              -> same_as_remove_cvref<typename RawHandleT::id_type>;
    { std::as_const(raw).id() }               -> same_as_remove_cvref<typename RawHandleT::id_type>;
    { static_cast<RawHandleT::id_type>(raw) } -> same_as_remove_cvref<typename RawHandleT::id_type>;
};






} // namespace detail
} // namespace josh
