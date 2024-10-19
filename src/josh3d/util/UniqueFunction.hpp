#pragma once
#include "CommonConcepts.hpp"
#include "AnyRef.hpp"
#include <cstddef>
#include <memory>
#include <cassert>
#include <type_traits>
#include <typeinfo>
#include <utility>



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
class UniqueFunction<ResT(ArgTs...)> {
private:
    using self_type = UniqueFunction<ResT(ArgTs...)>;

    class UFBase {
    public:
        virtual ResT operator()(ArgTs...) = 0;
        virtual const std::type_info& type() const noexcept = 0;
        virtual AnyRef      any_ref()       noexcept = 0;
        virtual AnyConstRef any_ref() const noexcept = 0;
        virtual ~UFBase() = default;
    };

    template<typename CallableT>
    class UFConcrete final : public UFBase {
    public:
        CallableT target;

        UFConcrete(CallableT&& target) : target{ std::move(target) } {}
        ResT operator()(ArgTs... args) override { return target(std::forward<ArgTs>(args)...); }

        const std::type_info& type() const noexcept override { return typeid(CallableT); }

        AnyRef      any_ref()       noexcept override { return AnyRef{ target }; }
        AnyConstRef any_ref() const noexcept override { return AnyConstRef{ target }; }

    };

    std::unique_ptr<UFBase> target_ptr_;


public:
    template<typename CallableT>
        requires
            not_move_or_copy_constructor_of<UniqueFunction, CallableT> &&
            of_signature<CallableT, ResT(ArgTs...)>
    UniqueFunction(CallableT&& callable)
        : target_ptr_{
            std::make_unique<
                UFConcrete<std::remove_reference_t<CallableT>>
            >(std::forward<CallableT>(callable))
        }
    {}

    ResT operator()(ArgTs... args) {
        assert(target_ptr_ && "UniqueFunction with no target has been invoked");
        return (*target_ptr_)(std::forward<ArgTs>(args)...);
    }


    AnyRef target_as_any() noexcept {
        assert(target_ptr_ && "UniqueFunction had no target");
        return target_ptr_->any_ref();
    }

    AnyConstRef target_as_any() const noexcept {
        assert(target_ptr_ && "UniqueFunction had no target");
        return target_ptr_->any_ref();
    }

    template<typename CallableT>
    CallableT* target_ptr() noexcept {
        return try_get_target<CallableT>();
    }

    template<typename CallableT>
    const CallableT* target_ptr() const noexcept {
        return try_get_target<CallableT>();
    }

    template<typename CallableT>
    CallableT& target_unchecked() noexcept {
        return get_unchecked<CallableT>();
    }

    template<typename CallableT>
    const CallableT& target_unchecked() const noexcept {
        return get_unchecked<CallableT>();
    }


    const std::type_info& target_type() const noexcept {
        assert(target_ptr_ && "UniqueFunction had no target");
        return target_ptr_->type();
    }

    operator bool() const noexcept { return bool(target_ptr_); }

    void swap(UniqueFunction& other) noexcept {
        std::swap(target_ptr_, other.target_ptr_);
    }

    UniqueFunction(const UniqueFunction&) = delete;
    UniqueFunction& operator=(const UniqueFunction&) = delete;

    UniqueFunction(UniqueFunction&& other) noexcept = default;
    UniqueFunction& operator=(UniqueFunction&& other) noexcept = default;

private:
    // Does not propagate constness to the target.
    // Use public target() functions for that.
    template<typename CallableT>
    CallableT* try_get_target() const noexcept {
        auto* wrapper_ptr = dynamic_cast<UFConcrete<CallableT>*>(target_ptr_.get());
        return wrapper_ptr ? &wrapper_ptr->target : nullptr;
    }

    template<typename CallableT>
    CallableT& get_unchecked() const noexcept {
        assert(target_ptr_ && "Requested the target of UniqueFunction with no target.");
        assert(target_ptr_->type() == typeid(CallableT) &&
            "Requested type does not match the type of the target.");
        return static_cast<UFConcrete<CallableT>&>(*target_ptr_).target;
    }
};


// ADL pls
template<typename ResT, typename ...ArgTs>
void swap(UniqueFunction<ResT(ArgTs...)>& lhs,
    UniqueFunction<ResT(ArgTs...)>& rhs) noexcept
{
    lhs.swap(rhs);
}

template<typename ResT, typename ...ArgTs>
bool operator==(const UniqueFunction<ResT(ArgTs...)>& fun,
    std::nullptr_t) noexcept
{
    return !fun;
}






} // namespace josh
