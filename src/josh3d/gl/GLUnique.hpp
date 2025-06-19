#pragma once
#include "CommonConcepts.hpp"
#include "GLAllocator.hpp"
#include "detail/TargetType.hpp"
#include "GLMutability.hpp"
#include <concepts>


namespace josh {


template<typename RawHandleT>
class GLShared;


template<typename RawHandleT>
class GLUnique
    : private GLAllocator<RawHandleT::kind_type>
    , public  detail::target_type_if_specified<RawHandleT>
{
public:
    static_assert(supports_gl_allocator<RawHandleT>);
    using                   handle_type    = RawHandleT;
    static constexpr GLKind kind_type      = handle_type::kind_type;
    using                   allocator_type = GLAllocator<kind_type>;

private:
    handle_type handle_;

    struct PrivateKey {};
    GLUnique(PrivateKey, handle_type::id_type id) : handle_(handle_type::from_id(id)) {}

    using mt            = mutability_traits<handle_type>;
    using mutability    = mt::mutability;
    using const_type    = mt::const_type;
    using mutable_type  = mt::mutable_type;
    using opposite_type = mt::opposite_type;
    using arg_type      = allocator_type::request_arg_type;

    template<typename T> friend class GLUnique; // For conversion through the underlying handle.
    template<typename T> friend class GLShared; // For sharing conversion since there's no release().

public:
    GLUnique() requires std::same_as<arg_type, void>
        : GLUnique(PrivateKey(), this->allocator_type::request())
    {}

    GLUnique() requires not_void<arg_type> and detail::specifies_target_type<handle_type>
        : GLUnique(PrivateKey(), this->allocator_type::request(handle_type::target_type))
    {}

    // The case for where the allocator takes an argument, but the argument value is
    // not known at compile time. So the argument has to be provided on construction.
    //
    // NOTE: The template is necessary otherwise for types with no argument this instantiates
    // `GLUnique(void arg)` which cannot compile even with `requires not_void<arg_type>`.
    template<std::same_as<arg_type> T>
        requires not_void<arg_type> and (not detail::specifies_target_type<handle_type>)
    GLUnique(T arg)
        : GLUnique(PrivateKey(), this->allocator_type::request(arg))
    {}

    // Will take ownership of the `handle`. Note that this is unsafe.
    static auto take_ownership(handle_type handle)
        -> GLUnique
    {
        return { PrivateKey(), handle.id() };
    }

    // Basic handle access.
    auto operator->() const noexcept -> const handle_type* { return &handle_; }
    auto operator*()  const noexcept -> const handle_type& { return handle_;  }
    auto get()        const noexcept -> handle_type        { return handle_;  }

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
    GLUnique& operator=(GLUnique&& other) noexcept
    {
        release_current();
        this->handle_ = std::exchange(other.handle_, handle_type::from_id(0));
        return *this;
    }

    // Converting move assignment.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLUnique& operator=(GLUnique<OtherHandleT>&& other) noexcept
    {
        release_current();
        this->handle_ = std::exchange(other.handle_, OtherHandleT::from_id(0));
        return *this;
    }

    ~GLUnique() noexcept { release_current(); }

private:
    void release_current() noexcept
    {
        if (handle_.id())
            this->allocator_type::release(handle_.id());
    }
};

/*
Override the mutabililty_traits for specializations of GLUnique so that the mutability
is inferred from the underlying Raw handle.
*/
template<template<typename...> typename RawTemplate, typename MutT, typename ...OtherTs>
    requires mutability_tag<MutT>
struct mutability_traits<GLUnique<RawTemplate<MutT, OtherTs...>>>
    : mutability_traits<RawTemplate<MutT, OtherTs...>>
{};


} // namespace josh



