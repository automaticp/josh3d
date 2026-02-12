#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "AnyRef.hpp"
#include "Semantics.hpp"
#include <memory>
#include <cassert>
#include <type_traits>
#include <typeinfo>



namespace josh {


/*
A type-erased function wrapper similar to std::move_only_function
and std::function with key features:

  - Guaranteed stable storage of the target callable accessed
    through target()/target_unchecked() member functions.
    It follows that there's no small buffer/object optimisation.

  - RTTI-enabled queries of the callable via target_type() and
    dynamic_cast-like target() function.

*/
template<typename Signature>
class UniqueFunction;

template<typename ResT, typename ...ArgTs>
class UniqueFunction<ResT(ArgTs...)>
    : public MoveOnly<UniqueFunction<ResT(ArgTs...)>>
{
public:
    using result_type = ResT;

    template<typename CallableT>
        requires
            not_move_or_copy_constructor_of<UniqueFunction, CallableT> and
            of_signature<CallableT, ResT(ArgTs...)>
    UniqueFunction(CallableT&& callable)
        : target_ptr_{ std::make_unique<Concrete<std::decay_t<CallableT>>>(FORWARD(callable)) }
    {}

    auto operator()(ArgTs... args)
        -> result_type
    {
        assert(target_ptr_ && "UniqueFunction with no target has been invoked.");
        return (*target_ptr_)(FORWARD(args)...);
    }

    auto target_as_any() noexcept
        -> AnyRef
    {
        assert(target_ptr_ && "UniqueFunction had no target.");
        return target_ptr_->any_ref();
    }

    auto target_as_any() const noexcept
        -> AnyConstRef
    {
        assert(target_ptr_ && "UniqueFunction had no target.");
        return target_ptr_->any_ref();
    }

    template<typename CallableT>
    auto target_ptr() noexcept -> CallableT* { return _try_get_target<CallableT>(); }
    template<typename CallableT>
    auto target_ptr() const noexcept -> const CallableT* { return _try_get_target<CallableT>(); }

    template<typename CallableT>
    auto target_unchecked() noexcept -> CallableT& { return _get_unchecked<CallableT>(); }
    template<typename CallableT>
    auto target_unchecked() const noexcept-> const CallableT& { return _get_unchecked<CallableT>(); }

    auto target_type() const noexcept
        -> const std::type_info&
    {
        assert(target_ptr_ && "UniqueFunction had no target.");
        return target_ptr_->type();
    }

    explicit operator bool() const noexcept { return bool(target_ptr_); }

private:
    struct Base
    {
        virtual auto operator()(ArgTs...) -> ResT = 0;
        virtual auto type()    const noexcept -> const std::type_info& = 0;
        virtual auto any_ref()       noexcept -> AnyRef      = 0;
        virtual auto any_ref() const noexcept -> AnyConstRef = 0;
        virtual ~Base() = default;
    };

    template<typename CallableT>
    struct Concrete final : Base
    {
        CallableT target;
        Concrete(CallableT&& target) : target{ MOVE(target) } {}
        auto operator()(ArgTs... args) -> ResT override { return target(FORWARD(args)...); }
        auto type()    const noexcept -> const std::type_info& override { return typeid(CallableT); }
        auto any_ref()       noexcept -> AnyRef      override { return AnyRef{ target }; }
        auto any_ref() const noexcept -> AnyConstRef override { return AnyConstRef{ target }; }
    };

    std::unique_ptr<Base> target_ptr_;

    // Does not propagate constness to the target.
    // Use public target() functions for that.
    template<typename CallableT>
    auto _try_get_target() const noexcept
        -> CallableT*
    {
        auto* wrapper_ptr = dynamic_cast<Concrete<CallableT>*>(target_ptr_.get());
        return wrapper_ptr ? &wrapper_ptr->target : nullptr;
    }

    template<typename CallableT>
    auto _get_unchecked() const noexcept
        -> CallableT&
    {
        assert(target_ptr_ && "Requested the target of UniqueFunction with no target.");
        assert(target_ptr_->type() == typeid(CallableT) && "Requested type does not match the type of the target.");
        return static_cast<Concrete<CallableT>&>(*target_ptr_).target;
    }
};


} // namespace josh
