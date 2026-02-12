#pragma once
#include "DefaultTextures.hpp"
#include "GLObjects.hpp"
#include "Resource.hpp"
#include "Scalars.hpp"


namespace josh {

/*
Material spec for the "Classic" Phong shading model.

NOTE: Not adding `color_factor`, `specular_factor`, etc.
because that's extra data to send when "instancing" and
I don't want to bother right now.

HMM: Textures are stored as-is, but it would probably
be better to have a specializaed storage for them too.
*/
struct MaterialPhong
{
    SharedConstTexture2D diffuse;   // [sRGB|sRGBA] Diffuse color.
    SharedConstTexture2D normal;    // [RGB] Tangent space normal map.
    SharedConstTexture2D specular;  // [R] Specular contribution factor.
    float                specpower; // That one parameter that nobody specifies.

    // TODO: This is pretty dumb, but is needed in the current system.
    ResourceUsage        diffuse_usage  = {};
    ResourceUsage        normal_usage   = {};
    ResourceUsage        specular_usage = {};

    // TODO: No idea how, but this is better be "moved outside" somehow.
    uintptr              aba_tag = {};
};

/*
Returns the material with textures set to global defaults,
no usage, and possibly custom ABA tag.

This is not done in the default init of the type itself as
it depends on the global state being initialized.
*/
[[nodiscard]]
inline auto make_default_material_phong(uintptr aba_tag = {})
    -> MaterialPhong
{
    // FIXME: I am not thrilled about forcefully sharing here
    // even if the user code will likely discard these later.
    // But for now, this is the simplest way to do it.
    // We'll likely move on to the texture pool later anyway.
    return {
        .diffuse   = globals::share_default_diffuse_texture(),
        .normal    = globals::share_default_normal_texture(),
        .specular  = globals::share_default_specular_texture(),
        .specpower = 128.f,
        .aba_tag   = aba_tag,
    };
}


} // namespace josh
