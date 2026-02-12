#pragma once
#include "AnyRef.hpp" // IWYU pragma: keep


#define JOSH3D_SIMPLE_STAGE_HOOK(StageName)        \
    namespace josh { struct StageName; }           \
    namespace josh::imguihooks {                   \
    struct StageName                               \
    {                                              \
        using target_stage_type = josh::StageName; \
        void operator()(AnyRef stage);             \
        void operator()(target_stage_type& stage); \
    };                                             \
    } // namespace josh::imguihooks

/*
Yes, this is dirty. No, not the macros. The cast from "Any" stage type to
a concrete one and how this basically is unavoidable.

I just worry about code that doesn't buy into this macro but still wants to
hook into stages. In any (hehe) case, just write a lambda that starts like this:

    [](AnyRef any_stage)
    {
        auto& stage = any_stage.target_unchecked<TargetType100PercentGuaranteeBroICheckedISwear>();

        // Do stuff with `stage`
        ...
    }

Then hope that wherever this lambda ends up the callbacks are invoked through the correct `type_index`.
*/
#define JOSH3D_SIMPLE_STAGE_HOOK_BODY(StageName)               \
    void josh::imguihooks::StageName::operator()(AnyRef stage) \
    {                                                          \
        (*this)(stage.target_unchecked<target_stage_type>());  \
    }                                                          \
    void josh::imguihooks::StageName::operator()(target_stage_type& stage)
    /* { Your code here. } */
