#pragma once


#define JOSH3D_SIMPLE_STAGE_HOOK(stage_type, stage_name)  \
    namespace josh::imguihooks::stage_type {              \
                                                          \
                                                          \
    class stage_name {                                    \
    private:                                              \
        stages::stage_type::stage_name& stage_;           \
                                                          \
    public:                                               \
        stage_name(stages::stage_type::stage_name& stage) \
            : stage_{ stage }                             \
        {}                                                \
                                                          \
        void operator()();                                \
    };                                                    \
                                                          \
                                                          \
    }


#define JOSH3D_SIMPLE_STAGE_HOOK_BODY(stage_type, stage_name) \
    void josh::imguihooks::stage_type::stage_name::operator()()
