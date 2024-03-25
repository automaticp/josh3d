#pragma once



/*
Strong integer type meant to be used at the call-site to disambiguate
integer paramters of certail funtions.

Compare weak integers:
    `fbo.attach_texture_layer_to_color_buffer(tex, 3, 1, 0);`

To using strong integer types:
    `fbo.attach_texture_layer_to_color_buffer(tex, Layer{ 3 }, 1, MipLevel{ 0 });`

*/
#define JOSH3D_DEFINE_STRONG_SCALAR(Name, Type)                      \
struct Name {                                                        \
    Type value;                                                      \
    constexpr explicit Name(Type value) noexcept : value{ value } {} \
    constexpr operator Type() const noexcept { return value; }       \
};



