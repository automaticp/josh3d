#include "Sky.hpp"
#include "Active.hpp"
#include "CubemapData.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLProgram.hpp"
#include "Geometry.hpp"
#include "ShaderPool.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "Skybox.hpp"
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>


namespace josh {


void Sky::operator()(RenderEnginePrimaryInterface& engine)
{
    if (sky_type == SkyType::None) return;

    glapi::disable(Capability::FaceCulling);
    glapi::set_depth_mask(false); // Disables writes to depth buffer.
    glapi::set_depth_test_condition(CompareOp::LEqual);

    switch (sky_type)
    {
        using enum SkyType;
        case Debug:      draw_debug_skybox  (engine);                    break;
        case Skybox:     draw_skybox        (engine, engine.registry()); break;
        case Procedural: draw_procedural_sky(engine, engine.registry()); break;
        case None: break;
    }

    glapi::set_depth_mask(true);
    glapi::set_depth_test_condition(CompareOp::Less);
}

void Sky::draw_debug_skybox(
    RenderEnginePrimaryInterface& engine)
{
    debug_skybox_cubemap_->bind_to_texture_unit(0);

    const BindGuard bcam = engine.bind_camera_ubo();
    const RawProgram<> sp = sp_skybox_.get();

    sp.uniform("cubemap", 0);

    const BindGuard bsp = sp.use();
    engine.draw([&](auto bfb)
    {
        engine.primitives().box_mesh().draw(bsp, bfb);
    });
}

void Sky::draw_skybox(
    RenderEnginePrimaryInterface& engine,
    const Registry&               registry)
{
    if (const CHandle skybox_handle = get_active<Skybox>(registry))
    {
        skybox_handle.get<Skybox>().cubemap->bind_to_texture_unit(0);

        const BindGuard bcam = engine.bind_camera_ubo();
        const RawProgram<> sp = sp_skybox_;

        sp.uniform("cubemap", 0);

        const BindGuard bsp = sp.use();
        engine.draw([&](auto bfb)
        {
            engine.primitives().box_mesh().draw(bsp, bfb);
        });
    }
    else
    {
        draw_procedural_sky(engine, registry); // Fallback.
    }
}

void Sky::draw_procedural_sky(
    RenderEnginePrimaryInterface& engine,
    const Registry&               registry)
{
    const RawProgram<> sp = sp_proc_;
    const BindGuard bcam = engine.bind_camera_ubo();

    if (const CHandle dlight = get_active<DirectionalLight, Transform>(registry))
    {
        // TODO: We should decompose_orientation() from the MTransform instead.
        // Oh god, this sounds like hell. WHY would you ever parent a directional light?!
        const vec3 light_dir =
            dlight.get<Transform>().orientation() * -Z;

        const vec3 light_dir_view_space =
            glm::normalize(vec3{ engine.camera_data().view * vec4{ light_dir, 0.f } });

        sp.uniform("sun_size_rad",         glm::radians(procedural_sky_params.sun_size_deg));
        sp.uniform("light_dir_view_space", light_dir_view_space);
        sp.uniform("sun_color",            procedural_sky_params.sun_color);
    }
    else
    {
        sp.uniform("sun_size_rad", 0.f); // Signals to not draw "sun".
    }

    sp.uniform("sky_color", procedural_sky_params.sky_color);

    const BindGuard bsp = sp.use();
    engine.draw([&](auto bfb)
    {
        engine.primitives().quad_mesh().draw(bsp, bfb);
    });
}

auto Sky::load_debug_skybox()
    -> UniqueCubemap
{
    // FIXME: This stupid and fragile. Really stupid.
    CubemapPixelData data =
        load_cubemap_pixel_data_from_json<pixel::RGB>(File("data/skyboxes/debug/skybox.json"));

    UniqueCubemap cube =
        create_skybox_from_cubemap_pixel_data(data, InternalFormat::SRGB8);

    cube->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
    return cube;
}


} // namespace josh
