#pragma once
#include "GLMutability.hpp" // IWYU pragma: keep
#include <utility>



/*
Oh boy... How am I going to explain this one?
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


