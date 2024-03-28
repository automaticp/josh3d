#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAllocator.hpp"
#include "GLMutability.hpp"
#include <concepts>           // IWYU pragma: keep
#include <type_traits>


namespace josh::dsa {


template<typename RawH>
concept specifies_target_type = requires {
    RawH::target_type;
    requires std::same_as<
        std::underlying_type_t<GLenum>,
        std::underlying_type_t<decltype(RawH::target_type)>
    >;
};


template<typename RawH>
    // requires allocatable_gl_kind_handle<RawH> || allocatable_gl_object_handle<RawH>
class GLUnique
    : public  RawH
    , private GLAllocator<RawH::kind_type>
{
private:
    using handle_type     = RawH;
    using allocator_type  = GLAllocator<RawH::kind_type>;

    using mt              = mutability_traits<RawH>;
    using mutability      = mt::mutability;
    using const_type      = mt::const_type;
    using mutable_type    = mt::mutable_type;
    using opposite_type   = mt::opposite_type;

    friend GLUnique<const_type>; // For GLMutable -> GLConst conversion

public:
    GLUnique() requires std::same_as<typename allocator_type::request_arg_type, void>
        : handle_type{ this->allocator_type::request() }
    {}

    GLUnique() requires std::same_as<typename allocator_type::request_arg_type, GLenum> && specifies_target_type<handle_type>
        : handle_type{ this->allocator_type::request(static_cast<GLenum>(handle_type::target_type)) }
    {}

    GLUnique(const GLUnique&)            = delete;
    GLUnique& operator=(const GLUnique&) = delete;

    GLUnique(GLUnique&& other) noexcept
        : handle_type{ other.handle_type::reset_id() }
    {}

    GLUnique& operator=(GLUnique&& other) noexcept {
        release_current();
        this->handle_type::reset_id(other.handle_type::reset_id());
        return *this;
    }

    // GLMutable -> GLConst converting move c-tor.
    GLUnique(GLUnique<mutable_type>&& other) noexcept
            requires gl_const<mutability>
        : handle_type{ other.handle_type::reset_id() }
    {}

    // GLMutable -> GLConst converting move assignment.
    GLUnique& operator=(GLUnique<mutable_type>&& other) noexcept
        requires gl_const<mutability>
    {
        release_current();
        this->handle_type::reset_id(other.handle_type::reset_id());
        return *this;
    }

    // GLConst -> GLMutable converting move construction is forbidden.
    GLUnique(GLUnique<const_type>&&) noexcept
        requires gl_mutable<mutability> = delete;

    // GLConst -> GLMutable converting move assignment is forbidden.
    GLUnique& operator=(GLUnique<const_type>&&) noexcept
        requires gl_mutable<mutability> = delete;


    ~GLUnique() noexcept { release_current(); }

private:
    void release_current() noexcept {
        if (this->handle_type::id()) {
            this->allocator_type::release(this->handle_type::id());
        }
    }
};



} // namespace josh::dsa


namespace josh {


template<template<typename...> typename RawTemplate, mutability_tag MutT, typename ...OtherTs>
struct mutability_traits<dsa::GLUnique<RawTemplate<MutT, OtherTs...>>>
    : mutability_traits<RawTemplate<MutT, OtherTs...>>
{};


} // namespace josh



