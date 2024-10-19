#pragma once
#include "AnyRef.hpp" // IWYU pragma: keep


#define JOSH3D_SIMPLE_STAGE_HOOK(stage_type, stage_name)                   \
    namespace josh::stages::stage_type {                                   \
        class stage_name;                                                  \
    }                                                                      \
                                                                           \
    namespace josh::imguihooks::stage_type {                               \
                                                                           \
                                                                           \
    class stage_name {                                                     \
    public:                                                                \
        using target_stage_type =                                          \
            stages::stage_type::stage_name;                                \
                                                                           \
        void operator()(AnyRef stage);                                     \
        void operator()(target_stage_type& stage);                         \
    };                                                                     \
                                                                           \
                                                                           \
    }


/*
Yes, this is dirty. No, not the macros. The cast from "Any" stage type to
a concrete one and how this basically is unavoidable.

I just worry about code that doesn't buy into this macro but still wants to
hook into stages. In any (hehe) case, just write a lambda that starts like this:

    [](AnyRef any_stage) {
        auto& stage = any_stage.target_unchecked<TargetType100PercentGuaranteeBroICheckedISwear>();

        // Do stuff with `stage`
        ...
    }

Then hope that wherever this lambda ends up the callbacks are invoked through the correct `type_index`.

TODO: The name of the stage parameter has an underscore at the end. Refactoring friction.
*/
#define JOSH3D_SIMPLE_STAGE_HOOK_BODY(stage_type, stage_name)                          \
    void josh::imguihooks::stage_type::stage_name::operator()(AnyRef stage) { \
        (*this)(stage.target_unchecked<target_stage_type>());                          \
    }                                                                                  \
                                                                                       \
    void josh::imguihooks::stage_type::stage_name::operator()(target_stage_type& stage_)
    /* { Your code here. } */
