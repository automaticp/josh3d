#pragma once
#include "CommonConcepts.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include <utility>


namespace josh {
namespace detail {


/*
The base class of all OpenGL handles. Represents a fully opaque
handle that has no knowledge of its type or allocation method.

NOTE: This is a mixin class It used to do more. Then I realized
it was dumb and was doing too much. Now it does less. But still mixin.
*/
template<typename IDType = GLuint>
class RawGLHandle
{
public:
    using id_type = IDType;
    explicit RawGLHandle(id_type id) : id_{ id } {}

    // Returns the underlying OpenGL ID (aka. Name) of the object.
    auto id() const noexcept -> id_type { return id_; }
    explicit operator id_type() const noexcept { return id_; }

    RawGLHandle(const RawGLHandle&)                = default;
    RawGLHandle(RawGLHandle&&) noexcept            = default;
    RawGLHandle& operator=(const RawGLHandle&)     = default;
    RawGLHandle& operator=(RawGLHandle&&) noexcept = default;

private:
    id_type id_;
    friend RawGLHandle<GLConst>;
protected:
    ~RawGLHandle() = default;
};

/*
Use this to indicate that a type is a Raw Handle type.
TODO: Incomplete and shaky, might be worth rethinking.
TODO: enable_raw_handle<T> trait?
*/
template<typename RawHandleT>
concept has_basic_raw_handle_semantics = requires(std::remove_cvref_t<RawHandleT> raw)
{
    typename RawHandleT::id_type;
    // Can return or be converted to the object id.
    { raw.id() }                              -> same_as_remove_cvref<typename RawHandleT::id_type>;
    { std::as_const(raw).id() }               -> same_as_remove_cvref<typename RawHandleT::id_type>;
    { static_cast<RawHandleT::id_type>(raw) } -> same_as_remove_cvref<typename RawHandleT::id_type>;
};


} // namespace detail
} // namespace josh
