#pragma once
#include "Active.hpp"
#include "CubemapData.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLProgram.hpp"
#include "ShaderPool.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "Skybox.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>




namespace josh::stages::primary {


class Sky {
public:
    enum class SkyType {
        None, Debug, Skybox, Procedural
    };

    struct ProceduralSkyParams {
        glm::vec3 sky_color{ 0.173f, 0.382f, 0.5f };
        glm::vec3 sun_color{ 1.f, 1.f, 1.f  };
        float     sun_size_deg{ 0.5f };
    };

    SkyType             sky_type{ SkyType::Procedural };
    ProceduralSkyParams procedural_sky_params{};

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    ShaderToken sp_skybox_ = shader_pool().get({
        .vert = VPath("src/shaders/skybox.vert"),
        .frag = VPath("src/shaders/skybox.frag")});

    ShaderToken sp_proc_ = shader_pool().get({
        .vert = VPath("src/shaders/sky_procedural.vert"),
        .frag = VPath("src/shaders/sky_procedural.frag")});

    UniqueCubemap debug_skybox_cubemap_ = load_debug_skybox();

    static UniqueCubemap load_debug_skybox();

    void draw_debug_skybox(
        RenderEnginePrimaryInterface& engine);

    void draw_skybox(
        RenderEnginePrimaryInterface& engine,
        const entt::registry&         registry);

    void draw_procedural_sky(
        RenderEnginePrimaryInterface& engine,
        const entt::registry&         registry);

};




inline void Sky::operator()(RenderEnginePrimaryInterface& engine) {

    if (sky_type == SkyType::None) { return; }

    glapi::disable(Capability::FaceCulling);
    glapi::set_depth_mask(false); // Disables writes to depth buffer.
    glapi::set_depth_test_condition(CompareOp::LEqual);

    switch (sky_type) {
        using enum SkyType;
        case Debug:      draw_debug_skybox  (engine);                    break;
        case Skybox:     draw_skybox        (engine, engine.registry()); break;
        case Procedural: draw_procedural_sky(engine, engine.registry()); break;
        default: break;
    }

    glapi::set_depth_mask(true);
    glapi::set_depth_test_condition(CompareOp::Less);
}




inline UniqueCubemap Sky::load_debug_skybox() {

    CubemapPixelData data =
        load_cubemap_pixel_data_from_json<pixel::RGB>(File("data/skyboxes/debug/skybox.json"));

    UniqueCubemap cube =
        create_skybox_from_cubemap_pixel_data(data, InternalFormat::SRGB8);

    cube->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
    return cube;
}




inline void Sky::draw_debug_skybox(
    RenderEnginePrimaryInterface& engine)
{
    debug_skybox_cubemap_->bind_to_texture_unit(0);

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();
    const RawProgram<> sp = sp_skybox_.get();

    sp.uniform("cubemap", 0);

    {
        BindGuard bound_program = sp.use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().box_mesh().draw(bound_program, bound_fbo);
        });
    }

}




inline void Sky::draw_skybox(
    RenderEnginePrimaryInterface& engine,
    const entt::registry&         registry)
{
    if (const auto skybox_handle = get_active<Skybox>(registry)) {

        skybox_handle.get<Skybox>().cubemap->bind_to_texture_unit(0);

        BindGuard bound_camera_ubo = engine.bind_camera_ubo();
        const RawProgram<> sp = sp_skybox_;

        sp.uniform("cubemap", 0);

        {
            BindGuard bound_program = sp.use();
            engine.draw([&](auto bound_fbo) {
                engine.primitives().box_mesh().draw(bound_program, bound_fbo);
            });
        }
    } else {
        draw_procedural_sky(engine, registry); // Fallback.
    }
}




inline void Sky::draw_procedural_sky(
    RenderEnginePrimaryInterface& engine,
    const entt::registry&         registry)
{
    const RawProgram<> sp = sp_proc_;
    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    if (const auto dlight = get_active<DirectionalLight, Transform>(registry)) {

        // TODO: We should decompose_orientation() from the MTransform instead.
        // Oh god, this sounds like hell. WHY would you ever parent a directional light?!
        const glm::vec3 light_dir =
            dlight.get<Transform>().orientation() * glm::vec3{ 0.f, 0.f, -1.f };

        const glm::vec3 light_dir_view_space =
            glm::normalize(glm::vec3{ engine.camera_data().view * glm::vec4{ light_dir, 0.f } });


        sp.uniform("sun_size_rad",         glm::radians(procedural_sky_params.sun_size_deg));
        sp.uniform("light_dir_view_space", light_dir_view_space);
        sp.uniform("sun_color",            procedural_sky_params.sun_color);
    } else {
        sp.uniform("sun_size_rad", 0.f); // Signals to not draw "sun".
    }

    sp.uniform("sky_color", procedural_sky_params.sky_color);

    {
        BindGuard bound_program = sp.use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }
}




} // namespace josh::stages::primary
