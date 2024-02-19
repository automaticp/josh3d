#pragma once
#include "detail/StageTraits.hpp" // IWYU pragma: keep


#define JOSH3D_SIMPLE_STAGE_HOOK(stage_type, stage_name)                   \
    namespace josh::imguihooks::stage_type {                               \
                                                                           \
                                                                           \
    class stage_name {                                                     \
    public:                                                                \
        using target_stage_type =                                          \
            stages::stage_type::stage_name;                                \
                                                                           \
        using any_stage_type =                                             \
            detail::stage_traits<detail::StageType::stage_type>::any_type; \
                                                                           \
                                                                           \
        void operator()(any_stage_type& stage);                            \
        void operator()(target_stage_type& stage);                         \
    };                                                                     \
                                                                           \
                                                                           \
    }


/*
Yes, this is dirty. No, not the macros. The cast from "Any" stage type to
a concrete one and how this basically is unavoidable.

We could get `void*` directly from UniqueFunction, is that cleaner for you?

It would be cleaner actually, we expect to know the concrete type by-construction
so there's no use for us to categorize stages as "AnyPrimaryStage", "AnyOverlayStage" etc.

I just worry about code that doesn't buy into this macro but still wants to
hook into stages. In any (hehe) case, just write a lambda that starts like this:

    [](AnyPrimaryStage& any_stage) {
        auto& stage = any_stage.target_unchecked<TargetType100PercentGuaranteeBroICheckedISwear>();

        // Do stuff with `stage`
        ...
    }

Then hope that wherever this lambda ends up the callbacks are invoked through the correct `type_index`.

TODO: Might rewrite to take void* instead later.
TODO: The name of the stage has an underscore at the end. Refactoring friction.
*/
#define JOSH3D_SIMPLE_STAGE_HOOK_BODY(stage_type, stage_name)                          \
    void josh::imguihooks::stage_type::stage_name::operator()(any_stage_type& stage) { \
        (*this)(stage.target_unchecked<target_stage_type>());                          \
    }                                                                                  \
                                                                                       \
    void josh::imguihooks::stage_type::stage_name::operator()(target_stage_type& stage_)
    /* { Your code here. } */
