#pragma once
#include "GLMutability.hpp" // IWYU pragma: keep
#include <utility>



/*
This defines a set of constructors and assignment operators
that define the core semantics of RawKindHandle and RawObject(Handle)
types.

This enables:
- Explicit construction from GLuint;
- Conversion from GLMutable to GLConst;
- No conversion from GLConst to GLMutable;

These constructors have to be redefined for each type separately
instead of pulling the base constructors with:
    using Base::Base;
because pulling the base constructors enables conversions between
unrelated types. For example, RawTexture2D and RawTexture2DMS become
directly interconvertible because they share a base c-tor:
    RawTextureHandle(const RawTextureHandle&);

What's worse, is that even completely unrelated "kinds" become
interconvertible through the common RawGLHandle base, so you can cast
RawTexture2D to RawVBO through
    RawGLHandle(const RawGLHandle&);
constructor, which is completely unacceptable and makes no sense.

So pulling the base c-tor and inheriting semantics from the
RawGLHandle base is not going to work, and instead we need
to just define c-tors for each class individually.

Instead of copy-pasting the same thing 23 times, there's this
macro that does exactly that. Wonderful.
*/
#define JOSH3D_MAGIC_CONSTRUCTORS(This, MutT, Parent)      \
    explicit This(GLuint id) noexcept : Parent{ id } {}    \
                                                           \
    This(const This&) = default;                           \
    This(This&&) noexcept = default;                       \
    This& operator=(const This&) = default;                \
    This& operator=(This&&) noexcept = default;            \
                                                           \
    This(const This<GLMutable>& other) noexcept            \
        requires gl_const<MutT>                            \
        : Parent{ other } {}                               \
                                                           \
    This& operator=(const This<GLMutable>& other) noexcept \
        requires gl_const<MutT>                            \
    {                                                      \
        Parent::operator=(other);                          \
        return *this;                                      \
    }                                                      \
                                                           \
    This(This<GLMutable>&& other) noexcept                 \
        requires gl_const<MutT>                            \
        : Parent{ std::move(other) } {}                    \
                                                           \
    This& operator=(This<GLMutable>&& other) noexcept      \
        requires gl_const<MutT>                            \
    {                                                      \
        Parent::operator=(std::move(other));               \
        return *this;                                      \
    }                                                      \
                                                           \
    This(const This<GLConst>&) noexcept                    \
        requires gl_mutable<MutT>                          \
    = delete;                                              \
                                                           \
    This& operator=(const This<GLConst>&) noexcept         \
        requires gl_mutable<MutT>                          \
    = delete;                                              \
                                                           \
    This(This<GLConst>&&) noexcept                         \
        requires gl_mutable<MutT>                          \
    = delete;                                              \
                                                           \
    This& operator=(This<GLConst>&&) noexcept              \
        requires gl_mutable<MutT>                          \
    = delete;


