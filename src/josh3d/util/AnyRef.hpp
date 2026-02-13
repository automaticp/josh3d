#pragma once
#include "CommonConcepts.hpp"
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <cassert>


namespace josh {


/*
I suppose this is similar to `Object` type in, say, Java, or just a plain ol' `void*` in C.

Now, hold your vomit for a moment.

This is used in polymorphic dispatch that is based on `type_index`. The invoked side in those
cases is supposed to know the concrete type anyway, and the invocation is guaranteed
by the selection of the correct `type_index`.

If you're not yet busy vigorously throwing up onto your desk, you might still be asking:
    "Why not use a visitor instead?".

Virtual polymorphism has this, arguably important, property where the polymorphic set
itself is unbounded and does not have to be known at compile time. This is convinient
for IoC, Callbacks, and linking in random dynamic libraries on those boring Thursdays.
In contrast to variant-based (or finite set) polymorphism that requires you to specify the
polymorphic set upfront, this is one of the major advantages of using virtual polymorphism.

Enter the visitor pattern, which at a first glance is a nice way of doing this "different
behaviours for the same polymorphic set", but it has this quirky little side-effect
of bringing you back into the realm of finite set polymorphism, meaning the primary
advantage of using virtual polymorphism goes out the window so much, that you might
as well start asking yourself why you chose to use it in the first place, as opposed to,
say, `std::variant`.

To preserve this unbounded-set -> unbounded-set mapping we can use `type_index`
and dispatch based on some `unordered_map<type_index, function<void(AnyRef)>>` or similar.

Hence, this:
*/
class AnyRef
{
public:
    template<typename T> requires not_move_or_copy_constructor_of<AnyRef, T>
    explicit AnyRef(T& object) noexcept
        : ptr_{ std::addressof(object) }
        , type_info_{ &typeid(object) }
    {}

    auto type() const noexcept -> const std::type_info& { return *type_info_; }
    auto type_index() const noexcept -> std::type_index { return { type() }; }

    auto target_void_ptr() const noexcept -> void* { return ptr_; }

    // UB if the types do not match.
    template<typename T>
    auto target_unchecked() const noexcept -> T&
    {
        assert(typeid(T) == type() && "Requested type does not match the type-erased target.");
        return *static_cast<T*>(ptr_);
    }

    // `nullptr` if the types do not match.
    template<typename T>
    auto target_ptr() const noexcept -> T*
    {
        return typeid(T) == type() ? *static_cast<T*>(ptr_) : nullptr;
    }

private:
    void*                 ptr_;
    const std::type_info* type_info_;

    friend class AnyConstRef; // For non-const -> const conversion.
};


class AnyConstRef
{
public:
    template<typename T> requires not_move_or_copy_constructor_of<AnyConstRef, T>
    explicit AnyConstRef(const T& object) noexcept
        : ptr_{ std::addressof(object) }
        , type_info_{ &typeid(object) }
    {}

    AnyConstRef(const AnyRef& other_ref) noexcept
        : ptr_{ other_ref.ptr_ }
        , type_info_{ other_ref.type_info_ }
    {}

    auto type() const noexcept -> const std::type_info& { return *type_info_; }
    auto type_index() const noexcept -> std::type_index { return { type() }; }

    auto target_void_ptr() const noexcept -> const void* { return ptr_; }

    // UB if the types do not match.
    template<typename T>
    auto target_unchecked() const noexcept -> const T&
    {
        assert(typeid(T) == type() && "Requested type does not match the type-erased target.");
        return *static_cast<const T*>(ptr_);
    }

    // `nullptr` if the types do not match.
    template<typename T>
    auto target_ptr() const noexcept -> const T*
    {
        return typeid(T) == type() ? *static_cast<const T*>(ptr_) : nullptr;
    }

private:
    const void*           ptr_;
    const std::type_info* type_info_;
};


} // namespace josh
