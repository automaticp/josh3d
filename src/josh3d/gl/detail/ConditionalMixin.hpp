#pragma once
#include <type_traits>


namespace josh::detail {


template<typename UniqueT>
struct NoMixin {};

template<bool ConditionV, typename IfTrue>
struct conditional_mixin : std::conditional<ConditionV, IfTrue, NoMixin<IfTrue>> {};

template<bool ConditionV, typename IfTrue>
using conditional_mixin_t = conditional_mixin<ConditionV, IfTrue>::type;


} // namespace josh::detail
