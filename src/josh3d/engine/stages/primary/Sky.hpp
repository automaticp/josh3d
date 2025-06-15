#pragma once
#include "EnumUtils.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "VPath.hpp"
#include "ECS.hpp"


namespace josh {


struct Sky
{
    enum class SkyType
    {
        None,
        Debug,
        Skybox,
        Procedural
    };
    SkyType sky_type = SkyType::Procedural;

    struct ProceduralSkyParams
    {
        vec3  sky_color = { 0.173f, 0.382f, 0.5f };
        vec3  sun_color = { 1.f, 1.f, 1.f };
        float sun_size_deg = 0.5f;
    };
    ProceduralSkyParams procedural_sky_params = {};

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    // TODO: Surely there are better ways, right?
    UniqueCubemap debug_skybox_cubemap_ = load_debug_skybox();

    static auto load_debug_skybox() -> UniqueCubemap;

    void draw_debug_skybox(
        RenderEnginePrimaryInterface& engine);

    void draw_skybox(
        RenderEnginePrimaryInterface& engine,
        const Registry&               registry);

    void draw_procedural_sky(
        RenderEnginePrimaryInterface& engine,
        const Registry&               registry);

    ShaderToken sp_skybox_ = shader_pool().get({
        .vert = VPath("src/shaders/skybox.vert"),
        .frag = VPath("src/shaders/skybox.frag")});

    ShaderToken sp_proc_ = shader_pool().get({
        .vert = VPath("src/shaders/sky_procedural.vert"),
        .frag = VPath("src/shaders/sky_procedural.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(Sky::SkyType, None, Debug, Skybox, Procedural);


} // namespace josh
