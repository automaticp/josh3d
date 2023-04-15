#pragma once
#include <concepts>
#include <memory>
#include <functional>
#include <type_traits>
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
        virtual ~UFBase() = default;
    };

    template<typename CallableT>
    class UFConcrete : public UFBase {
    private:
        CallableT target_;
    public:
        UFConcrete(CallableT&& target) : target_{ std::move(target) } {}
        ResT operator()(ArgTs... args) override {
            return std::invoke(target_, std::forward<ArgTs>(args)...);
        }
    };

    std::unique_ptr<UFBase> target_ptr_;


public:
    template<typename CallableT>
        requires (!std::same_as<std::remove_cvref_t<CallableT>, self_type>)
    UniqueFunction(CallableT&& callable)
        : target_ptr_{
            std::make_unique<
                UFConcrete<std::remove_reference_t<CallableT>>
            >(std::forward<CallableT>(callable))
        }
    {
        using value_type = std::remove_cvref_t<CallableT>;
        static_assert(std::is_nothrow_move_constructible_v<value_type>);
        static_assert(std::is_nothrow_move_assignable_v<value_type>);
    }

    ResT operator()(ArgTs... args) {
        return std::invoke(*target_ptr_, std::forward<ArgTs>(args)...);
    }

    UniqueFunction(const UniqueFunction&) = delete;
    UniqueFunction& operator=(const UniqueFunction&) = delete;

    UniqueFunction(UniqueFunction&& other) noexcept = default;
    UniqueFunction& operator=(UniqueFunction&& other) noexcept = default;
};









} // namespace learn
