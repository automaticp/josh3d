#pragma once


/*
Reducing boilerplate in object_api mixins.
*/
#define JOSH3D_MIXIN_HEADER \
    private:                \
        auto self() const -> const CRTP& { return static_cast<const CRTP&>(*this); } \
        auto self_id() const noexcept -> GLuint { return self().id(); }              \
        using mt = mutability_traits<CRTP>;                                          \
    public:
