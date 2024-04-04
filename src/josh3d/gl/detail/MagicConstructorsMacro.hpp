#pragma once
#include "GLMutability.hpp" // IWYU pragma: keep
#include <utility>          // IWYU pragma: keep



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



/*
Macro used to escape commas "," in macro arguments.

Could probably be moved somewhere else.
*/
#define JOSH3D_SINGLE_ARG(...) __VA_ARGS__




#define JOSH3D_MAGIC_CONSTRUCTORS_CONVERSION(Self, MT, Parent)                                                              \
    Self(const Self&) noexcept            = default;                                                                        \
    Self(Self&&) noexcept                 = default;                                                                        \
    Self& operator=(const Self&) noexcept = default;                                                                        \
    Self& operator=(Self&&) noexcept      = default;                                                                        \
                                                                                                                            \
    Self(const MT::mutable_type& other) noexcept requires gl_const<typename MT::mutability> : Parent{ other } {}            \
    Self(MT::mutable_type&& other) noexcept      requires gl_const<typename MT::mutability> : Parent{ std::move(other) } {} \
    Self& operator=(const MT::mutable_type& other) noexcept requires gl_const<typename MT::mutability> {                    \
        Parent::operator=(other);                                                                                           \
        return *this;                                                                                                       \
    }                                                                                                                       \
    Self& operator=(MT::mutable_type&& other) noexcept      requires gl_const<typename MT::mutability> {                    \
        Parent::operator=(std::move(other));                                                                                \
        return *this;                                                                                                       \
    }                                                                                                                       \
    Self(const MT::const_type&) noexcept            requires gl_mutable<typename MT::mutability> = delete;                  \
    Self(MT::const_type&&) noexcept                 requires gl_mutable<typename MT::mutability> = delete;                  \
    Self& operator=(const MT::const_type&) noexcept requires gl_mutable<typename MT::mutability> = delete;                  \
    Self& operator=(MT::const_type&&) noexcept      requires gl_mutable<typename MT::mutability> = delete;


#define JOSH3D_MAGIC_CONSTRUCTORS_FROM_ID(Self, Parent)                       \
private:                                                                      \
    explicit Self(Parent::id_type id) noexcept : Parent{ id } {}              \
public:                                                                       \
    using id_type = Parent::id_type;                                          \
    static constexpr Self from_id(id_type id) noexcept { return Self{ id }; } \




/*
Alternative version of the macro above that works with types
that have extra template arguments besides mutability.

Self   - Name of the template type as within the class body, without the template arguments.
MT     - mutability_traits of the type. Equivalent to mutability_traits<Self> within class body.
Parent - parent type to delegate construction to. Most likely is RawGLHandle<MutT>.
*/
#define JOSH3D_MAGIC_CONSTRUCTORS_2(Self, MT, Parent)                                                               \
    JOSH3D_MAGIC_CONSTRUCTORS_FROM_ID(JOSH3D_SINGLE_ARG(Self), JOSH3D_SINGLE_ARG(Parent))                           \
    JOSH3D_MAGIC_CONSTRUCTORS_CONVERSION(JOSH3D_SINGLE_ARG(Self), JOSH3D_SINGLE_ARG(MT), JOSH3D_SINGLE_ARG(Parent))
