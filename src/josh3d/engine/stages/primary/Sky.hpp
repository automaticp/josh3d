#pragma once
#include "CubemapData.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "TextureHelpers.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "components/Skybox.hpp"
#include "VPath.hpp"
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

    SkyType             sky_type{ SkyType::Skybox };
    ProceduralSkyParams procedural_sky_params{};

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    UniqueProgram sp_skybox_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/skybox.vert"))
            .load_frag(VPath("src/shaders/skybox.frag"))
            .get()
    };

    UniqueProgram sp_proc_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/sky_procedural.vert"))
            .load_frag(VPath("src/shaders/sky_procedural.frag"))
            .get()
    };

    UniqueCubemap debug_skybox_cubemap_ = load_debug_skybox();

    static UniqueCubemap load_debug_skybox();

    void draw_debug_skybox(
        RenderEnginePrimaryInterface& engine);

    void draw_skybox(
        RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    void draw_procedural_sky(
        RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

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
        load_cubemap_from_json<pixel::RGB>(File("data/skyboxes/debug/skybox.json"));

    UniqueCubemap cube =
        create_skybox_from_cubemap_data(data, InternalFormat::SRGB8);

    cube->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
    return cube;
}




inline void Sky::draw_debug_skybox(
    RenderEnginePrimaryInterface& engine)
{

    glm::mat4 projection = engine.camera().projection_mat();
    glm::mat4 view       = engine.camera().view_mat();
    debug_skybox_cubemap_->bind_to_texture_unit(0);

    sp_skybox_->uniform("cubemap",    0);
    sp_skybox_->uniform("projection", projection);
    sp_skybox_->uniform("view",       glm::mat4{ glm::mat3{ view } });

    {
        BindGuard bound_program = sp_skybox_->use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().box_mesh().draw(bound_program, bound_fbo);
        });
    }

}




inline void Sky::draw_skybox(
    RenderEnginePrimaryInterface& engine,
    const entt::registry&         registry)
{

    glm::mat4 projection = engine.camera().projection_mat();
    glm::mat4 view       = engine.camera().view_mat();

    // TODO: Pulls a single skybox, obviously won't work when there are many.
    registry.storage<components::Skybox>().begin()->cubemap->bind_to_texture_unit(0);

    sp_skybox_->uniform("cubemap",    0);
    sp_skybox_->uniform("projection", projection);
    sp_skybox_->uniform("view",       glm::mat4{ glm::mat3{ view } });

    {
        BindGuard bound_program = sp_skybox_->use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().box_mesh().draw(bound_program, bound_fbo);
        });
    }

}




inline void Sky::draw_procedural_sky(
    RenderEnginePrimaryInterface& engine,
    const entt::registry&         registry)
{

    const auto& cam        = engine.camera();
    const auto& cam_params = cam.get_params();

    const glm::mat4 inv_proj = glm::inverse(cam.projection_mat());

    // UB if no light, lmao
    const auto& light = *registry.storage<light::Directional>().begin();

    const glm::vec3 light_dir_view_space =
        glm::normalize(glm::vec3{ cam.view_mat() * glm::vec4{ light.direction, 0.f } });

    sp_proc_->uniform("z_far",                cam_params.z_far);
    sp_proc_->uniform("inv_proj",             inv_proj);
    sp_proc_->uniform("light_dir_view_space", light_dir_view_space);
    sp_proc_->uniform("sky_color",            procedural_sky_params.sky_color);
    sp_proc_->uniform("sun_color",            procedural_sky_params.sun_color);
    sp_proc_->uniform("sun_size_rad",         glm::radians(procedural_sky_params.sun_size_deg));

    {
        BindGuard bound_program = sp_proc_->use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }
}




} // namespace josh::stages::primary
