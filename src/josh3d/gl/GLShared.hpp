#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAllocator.hpp"
#include "detail/TargetType.hpp"
#include "GLKind.hpp"
#include "GLMutability.hpp"
#include <concepts>           // IWYU pragma: keep
#include <cstdint>
#include <atomic>
#include <utility>


namespace josh {


template<supports_gl_allocator RawHandleT>
class GLShared
    : private GLAllocator<RawHandleT::kind_type>
    , public  josh::detail::target_type_if_specified<RawHandleT>
{
public:
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

    friend GLShared<const_type>; // For GLMutable -> GLConst conversion

    // The state of the GLShared consists of two components:
    // 1. Integer ID stored in the `handle_type` base class;
    // 2. Pointer to reference count block in the `control_block_` member variable.
    // This is similar to `shared_ptr` except that the pointer-to-storage is
    // not a pointer at all, but an OpenGL object "name".

    struct RefCount {
        std::atomic<std::size_t> count_{ 1 };
    };

    RefCount* control_block_;

    GLShared(handle_type::id_type id)
        : handle_       { handle_type::from_id(id) }
        , control_block_{ new RefCount{ 1 }        }
    {}

public:
    // Default constructor overload for allocating unspecialized objects.
    GLShared()
        requires
            std::same_as<typename allocator_type::request_arg_type, void>
        : GLShared(this->allocator_type::request())
    {}

    // Default constructor overload for allocating target-specialized objects.
    GLShared()
        requires
            std::same_as<typename allocator_type::request_arg_type, GLenum> &&
            josh::detail::specifies_target_type<handle_type>
        : GLShared(this->allocator_type::request(static_cast<GLenum>(handle_type::target_type)))
    {}



    // Basic handle access.
    const handle_type* operator->() const noexcept { return &handle_; }
    const handle_type& operator*()  const noexcept { return handle_;  }
    handle_type        get()        const noexcept { return handle_;  }




    // Returns a unique id associated with the managed resource.
    // For equality comparison and maybe hashing, nothing else has concrete meaning.
    // Will return 0 if the object is in moved-from state.
    std::uintptr_t shared_block_id() const noexcept {
        return reinterpret_cast<std::uintptr_t>(control_block_);
    }

    // Returns the number of instances holding ownership over the resource.
    // Is only a hint and should not be relied upon **even if** the returned result is 1.
    // (The use count load is performed with relaxed memory order).
    // Prefer `shared_only_owner()` intead for that case.
    // Will return 0 if the object is in moved-from state.
    std::size_t shared_use_count_hint() const noexcept {
        if (control_block_) {
            return control_block_->count_.load(std::memory_order_relaxed);
        } else {
            return 0;
        }
    }

    // Returns `true` if this instance is the only owner of the resource, `false` otherwise.
    // Note that this is different from `std::shared_ptr::unique()` and is reliable in multithreaded environments.
    // (The use count load is performed with acquire memory order).
    // Will return `false` if the object is in moved-from state.
    bool shared_only_owner() const noexcept {
        if (control_block_) {
            return control_block_->count_.load(std::memory_order_acquire) == 1;
        } else {
            return false;
        }
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

    // GLMutable -> GLConst converting copy c-tor.
    GLShared(const GLShared<mutable_type>& other) noexcept
            requires mt::is_const
        : handle_       { other.handle_        }
        , control_block_{ other.control_block_ }
    {
        acquire_ownership();
    }

    // GLConst -> GLMutable converting copy construction is forbidden.
    GLShared(const GLShared<const_type>& other) noexcept
        requires mt::is_mutable = delete;




    // Move c-tor.
    GLShared(GLShared&& other) noexcept
        : handle_       { std::exchange(other.handle_, handle_type::from_id(0)) }
        , control_block_{ std::exchange(other.control_block_, nullptr) }
    {}

    // GLMutable -> GLConst converting move c-tor.
    GLShared(GLShared<mutable_type>&& other) noexcept
            requires mt::is_const
        : handle_       { std::exchange(other.handle_, mutable_type::from_id(0)) }
        , control_block_{ std::exchange(other.control_block_, nullptr) }
    {}

    // GLConst -> GLMutable converting move construction is forbidden.
    GLShared(GLShared<const_type>&&) noexcept
        requires mt::is_mutable = delete;



    // Copy assignment operator.
    GLShared& operator=(const GLShared& other) noexcept {
        if (this != &other) {
            release_ownership(); // Previous resource.

            handle_        = other.handle_;
            control_block_ = other.control_block_;

            acquire_ownership(); // New resource.
        }
        return *this;
    }

    // GLMutable -> GLConst converting copy assignment.
    GLShared& operator=(const GLShared<mutable_type>& other) noexcept
        requires mt::is_const
    {
        if (this != &other) { // TODO: isn't this check redundunt here?
            release_ownership(); // Previous resource.

            handle_        = other.handle_;
            control_block_ = other.control_block_;

            acquire_ownership(); // New resource.
        }
        return *this;
    }

    // GLConst -> GLMutable converting copy assignment is forbidden.
    GLShared& operator=(const GLShared<const_type>& other) noexcept
        requires mt::is_mutable = delete;



    // Move assignment operator.
    GLShared& operator=(GLShared&& other) noexcept {
        release_ownership(); // Previous resource.

        handle_        = std::exchange(other.handle_, handle_type::from_id(0));
        control_block_ = std::exchange(other.control_block_, nullptr);

        return *this;
    }

    // GLMutable -> GLConst converting move assignment.
    GLShared& operator=(GLShared<mutable_type>&& other) noexcept
        requires mt::is_const
    {
        release_ownership(); // Previous resource.

        handle_        = std::exchange(other.handle_, mutable_type::from_id(0));
        control_block_ = std::exchange(other.control_block_, nullptr);

        return *this;
    }

    // GLConst -> GLMutable converting move assignment is forbidden.
    GLShared& operator=(GLShared<const_type>&& other) noexcept
        requires mt::is_mutable = delete;




    ~GLShared() noexcept { release_ownership(); }

private:
    void acquire_ownership() noexcept {
        if (control_block_) {
            control_block_->count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void release_ownership() noexcept {
        if (control_block_ &&
            control_block_->count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete control_block_;
            release_resource();
        }
    }

    void release_resource() noexcept {
        if (handle_.id()) {
            this->allocator_type::release(handle_.id());
        }
    }
};




} // namespace josh
namespace josh {

// Override the mutabililty_traits for specializations of GLShared so that the mutability
// is inferred from the underlying Raw handle.
template<template<typename...> typename RawTemplate, mutability_tag MutT, typename ...OtherTs>
struct mutability_traits<GLShared<RawTemplate<MutT, OtherTs...>>>
    : mutability_traits<RawTemplate<MutT, OtherTs...>>
{};


} // namespace josh


