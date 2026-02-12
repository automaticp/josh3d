#pragma once
#include "CommonConcepts.hpp"
#include "GLAllocator.hpp"
#include "GLUnique.hpp"
#include "detail/TargetType.hpp"
#include "GLKind.hpp"
#include "GLMutability.hpp"
#include <concepts>
#include <cstdint>
#include <atomic>
#include <utility>


namespace josh {
namespace detail {
/*
The control block is defined outside of GLShared, so that
it could be passed between different specializations of GLShared
(ex. when doing mutable -> const conversion).
*/
struct ControlBlock
{
    std::atomic<std::size_t> count = { 1 };
};
} // namespace detail


template<typename RawHandleT>
class GLShared
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
    using arg_type        = allocator_type::request_arg_type;

    template<typename T> friend class GLShared;

    // The state of the GLShared consists of two components:
    // 1. Integer ID stored in the `handle_type` base class;
    // 2. Pointer to reference count block in the `control_block_` member variable.
    // This is similar to `shared_ptr` except that the pointer-to-storage is
    // not a pointer at all, but an OpenGL object "name".

    detail::ControlBlock* control_block_;

    struct PrivateKey {};
    GLShared(PrivateKey, handle_type::id_type id)
        : handle_       { handle_type::from_id(id)      }
        , control_block_{ new detail::ControlBlock{ 1 } }
    {}

public:
    GLShared() requires std::same_as<arg_type, void>
        : GLShared(PrivateKey(), this->allocator_type::request())
    {}

    GLShared() requires not_void<arg_type> and detail::specifies_target_type<handle_type>
        : GLShared(PrivateKey(), this->allocator_type::request(handle_type::target_type))
    {}

    // The case for where the allocator takes an argument, but the argument value is
    // not known at compile time. So the argument has to be provided on construction.
    //
    // NOTE: The template is necessary otherwise for types with no argument this instantiates
    // `GLShared(void arg)` which cannot compile even with `requires not_void<arg_type>`.
    template<std::same_as<arg_type> T>
        requires not_void<arg_type> and (not detail::specifies_target_type<handle_type>)
    GLShared(T arg)
        : GLShared(PrivateKey(), this->allocator_type::request(arg))
    {}

    // Basic handle access.
    auto operator->() const noexcept -> const handle_type* { return &handle_; }
    auto operator*()  const noexcept -> const handle_type& { return handle_;  }
    auto get()        const noexcept -> handle_type        { return handle_;  }

    // Returns a unique id associated with the managed resource.
    // For equality comparison and maybe hashing, nothing else has concrete meaning.
    // Will return 0 if the object is in moved-from state.
    auto shared_block_id() const noexcept
        -> std::uintptr_t
    {
        return reinterpret_cast<std::uintptr_t>(control_block_);
    }

    // Returns the number of instances holding ownership over the resource.
    // Is only a hint and should not be relied upon **even if** the returned result is 1.
    // (The use count load is performed with relaxed memory order).
    // Prefer `shared_only_owner()` intead for that case.
    // Will return 0 if the object is in moved-from state.
    auto shared_use_count_hint() const noexcept
        -> std::size_t
    {
        if (control_block_)
            return control_block_->count.load(std::memory_order_relaxed);

        return 0;
    }

    // Returns `true` if this instance is the only owner of the resource, `false` otherwise.
    // Note that this is different from `std::shared_ptr::unique()` and is reliable in multithreaded environments.
    // (The use count load is performed with acquire memory order).
    // Will return `false` if the object is in moved-from state.
    auto shared_only_owner() const noexcept
        -> bool
    {
        if (control_block_)
            return control_block_->count.load(std::memory_order_acquire) == 1;

        return false;
    }

    // Implicit conversion to owned raw handles.
    // The && operators are deleted to prevent "slicing".
    operator const_type()   const&  noexcept requires convertible_mutability_to<mutability, GLConst>   { return handle_; }
    operator mutable_type() const&  noexcept requires convertible_mutability_to<mutability, GLMutable> { return handle_; }
    operator const_type()   const&& noexcept = delete;
    operator mutable_type() const&& noexcept = delete;

    // Copy c-tor.
    GLShared(const GLShared& other) noexcept
        : handle_       { other.handle_        }
        , control_block_{ other.control_block_ }
    {
        acquire_ownership();
    }

    // Converting copy c-tor.
    template<std::convertible_to<RawHandleT> OtherRawT>
    GLShared(const GLShared<OtherRawT>& other) noexcept
        : handle_       { other.handle_        }
        , control_block_{ other.control_block_ }
    {
        acquire_ownership();
    }

    // Move c-tor.
    GLShared(GLShared&& other) noexcept
        : handle_       { std::exchange(other.handle_,        handle_type::from_id(0)) }
        , control_block_{ std::exchange(other.control_block_, nullptr)                 }
    {}

    // Converting move c-tor.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLShared(GLShared<OtherHandleT>&& other) noexcept
        : handle_       { std::exchange(other.handle_,        OtherHandleT::from_id(0)) }
        , control_block_{ std::exchange(other.control_block_, nullptr)                  }
    {}

    // Sharing conversion from GLUnique.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLShared(GLUnique<OtherHandleT>&& other) noexcept
        : GLShared{ PrivateKey{}, std::exchange(other.handle_, OtherHandleT::from_id(0)).id() }
    {}

    // Copy assignment operator.
    GLShared& operator=(const GLShared& other) noexcept
    {
        if (this != &other)
        {
            release_ownership(); // Previous resource.

            handle_        = other.handle_;
            control_block_ = other.control_block_;

            acquire_ownership(); // New resource.
        }
        return *this;
    }

    // Converting copy assignment operator.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLShared& operator=(const GLShared<OtherHandleT>& other) noexcept
    {
        release_ownership(); // Previous resource.

        handle_        = other.handle_;
        control_block_ = other.control_block_;

        acquire_ownership(); // New resource.

        return *this;
    }

    // Move assignment operator.
    GLShared& operator=(GLShared&& other) noexcept
    {
        release_ownership(); // Previous resource.

        handle_        = std::exchange(other.handle_, handle_type::from_id(0));
        control_block_ = std::exchange(other.control_block_, nullptr);

        return *this;
    }

    // Converting move assignment operator.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLShared& operator=(GLShared<OtherHandleT>&& other) noexcept
    {
        release_ownership(); // Previous resource.

        handle_        = std::exchange(other.handle_,        OtherHandleT::from_id(0));
        control_block_ = std::exchange(other.control_block_, nullptr);

        return *this;
    }

    // Sharing assignment from GLUnique.
    template<std::convertible_to<RawHandleT> OtherHandleT>
    GLShared& operator=(GLUnique<OtherHandleT>&& other) noexcept
    {
        // Construct from GLUnique and then assign to self.
        this->operator=(GLShared(std::move(other)));
        return *this;
    }

    ~GLShared() noexcept { release_ownership(); }

private:
    void acquire_ownership() noexcept
    {
        if (control_block_)
            control_block_->count.fetch_add(1, std::memory_order_relaxed);
    }

    void release_ownership() noexcept
    {
        if (control_block_ and
            control_block_->count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete control_block_;
            release_resource();
        }
    }

    void release_resource() noexcept
    {
        if (handle_.id())
            this->allocator_type::release(handle_.id());
    }
};

// Override the mutabililty_traits for specializations of GLShared so that the mutability
// is inferred from the underlying Raw handle.
template<template<typename...> typename RawTemplate, mutability_tag MutT, typename ...OtherTs>
struct mutability_traits<GLShared<RawTemplate<MutT, OtherTs...>>>
    : mutability_traits<RawTemplate<MutT, OtherTs...>>
{};


} // namespace josh


