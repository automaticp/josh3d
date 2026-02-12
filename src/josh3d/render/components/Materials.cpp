#include "Materials.hpp"
#include "DefaultTextures.hpp"


namespace josh {


auto make_default_material_phong(uintptr aba_tag) -> MaterialPhong
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
