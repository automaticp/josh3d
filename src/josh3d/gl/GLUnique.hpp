#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAllocator.hpp"
#include "detail/TargetType.hpp"
#include "GLMutability.hpp"
#include <concepts>           // IWYU pragma: keep
#include <type_traits>


namespace josh {


template<typename RawHandleT>
class GLShared;



template<typename RawHandleT>
class GLUnique
    : private GLAllocator<RawHandleT::kind_type>
    , public  josh::detail::target_type_if_specified<RawHandleT>
{
public:
    static_assert(supports_gl_allocator<RawHandleT>);
    using                   handle_type    = RawHandleT;
    static constexpr GLKind kind_type      = handle_type::kind_type;
    using                   allocator_type = GLAllocator<kind_type>;

private:
    handle_type handle_;

    using mt              = mutability_traits<handle_type>;
    using mutability      = mt::mutability;
    using const_type      = mt::const_type;
    using mutable_type    = mt::mutable_type;
    using opposite_type   = mt::opposite_type;

    template<typename T> friend class GLUnique; // For conversion through the underlying handle.
    template<typename T> friend class GLShared; // For sharing conversion since there's no release().

public:
    GLUnique()
        requires
            std::same_as<typename allocator_type::request_arg_type, void>
        : handle_{ handle_type::from_id(this->allocator_type::request()) }
    {}

    GLUnique()
        requires
            std::same_as<typename allocator_type::request_arg_type, GLenum> &&
            josh::detail::specifies_target_type<handle_type>
        : handle_{ handle_type::from_id(this->allocator_type::request(static_cast<GLenum>(handle_type::target_type))) }
    {}



    // Basic handle access.
    const handle_type* operator->() const noexcept { return &handle_; }
    const handle_type& operator*()  const noexcept { return handle_;  }
    handle_type        get()        const noexcept { return handle_;  }




    // Implicit conversion to owned raw handles.
    // The && operators are deleted to prevent "slicing".
    operator const_type()   const&  noexcept requires convertible_mutability_to<mutability, GLConst>   { return handle_; }
    operator mutable_type() const&  noexcept requires convertible_mutability_to<mutability, GLMutable> { return handle_; }
    operator const_type()   const&& noexcept = delete;
    operator mutable_type() const&& noexcept = delete;




    // No copy.
    GLUnique(const GLUnique&)            = delete;
    GLUnique& operator=(const GLUnique&) = delete;


    // Move c-tor.
    GLUnique(GLUnique&& other) noexcept
        : handle_{ std::exchange(other.handle_, handle_type::from_id(0)) }
    {}

    // Converting move c-tor.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLUnique(GLUnique<OtherHandleT>&& other) noexcept
        : handle_{ std::exchange(other.handle_, OtherHandleT::from_id(0)) }
    {}


    // Move assignment.
    GLUnique& operator=(GLUnique&& other) noexcept {
        release_current();
        this->handle_ = std::exchange(other.handle_, handle_type::from_id(0));
        return *this;
    }

    // Converting move assignment.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLUnique& operator=(GLUnique<OtherHandleT>&& other) noexcept {
        release_current();
        this->handle_ = std::exchange(other.handle_, OtherHandleT::from_id(0));
        return *this;
    }


    ~GLUnique() noexcept { release_current(); }

private:
    void release_current() noexcept {
        if (handle_.id()) {
            this->allocator_type::release(handle_.id());
        }
    }
};



} // namespace josh
namespace josh {


// Override the mutabililty_traits for specializations of GLUnique so that the mutability
// is inferred from the underlying Raw handle.
template<template<typename...> typename RawTemplate, typename MutT, typename ...OtherTs>
    requires mutability_tag<MutT>
struct mutability_traits<GLUnique<RawTemplate<MutT, OtherTs...>>>
    : mutability_traits<RawTemplate<MutT, OtherTs...>>
{};


} // namespace josh



