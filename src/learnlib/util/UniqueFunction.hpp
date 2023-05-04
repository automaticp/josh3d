#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <cassert>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <utility>



namespace learn {


// Simplified move_only_function implementation because I JUST CANNOT.
// TODO: Most likely broken in some way, so check later.

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
        virtual ~UFBase() = default;
    };

    template<typename CallableT>
    class UFConcrete final : public UFBase {
    public:
        CallableT target;

        UFConcrete(CallableT&& target) : target{ std::move(target) } {}
        ResT operator()(ArgTs... args) override {
            return std::invoke(target, std::forward<ArgTs>(args)...);
        }

        const std::type_info& type() const noexcept override {
            return typeid(CallableT);
        }

    };

    std::unique_ptr<UFBase> target_ptr_;


public:
    template<typename CallableT>
        requires (
            !std::same_as<std::remove_cvref_t<CallableT>, self_type> &&
            std::invocable<CallableT, ArgTs...> &&
            std::same_as<std::invoke_result_t<CallableT, ArgTs...>, ResT>
        )
    UniqueFunction(CallableT&& callable)
        : target_ptr_{
            std::make_unique<
                UFConcrete<std::remove_reference_t<CallableT>>
            >(std::forward<CallableT>(callable))
        }
    {}

    ResT operator()(ArgTs... args) {
        assert(target_ptr_ && "UniqueFunction with no target has been invoked");
        return std::invoke(*target_ptr_, std::forward<ArgTs>(args)...);
    }

    template<typename CallableT>
    CallableT* target() noexcept {
        return try_get_target<CallableT>();
    }

    template<typename CallableT>
    const CallableT* target() const noexcept {
        return try_get_target<CallableT>();
    }

    const std::type_info& target_type() const noexcept {
        if (!target_ptr_) { return typeid(void); }
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






} // namespace learn
