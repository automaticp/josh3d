#include "DeferredShading.hpp"
#include "Active.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLAPICore.hpp"
#include "GLProgram.hpp"
#include "LightsGPU.hpp"
#include "Mesh.hpp"
#include "Tags.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp"
#include "LightCasters.hpp"
#include "BoundingSphere.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "stages/primary/SSAO.hpp"
#include "ShadowCasting.hpp"
#include "StageContext.hpp"
#include "Visible.hpp"
#include "Tracy.hpp"
#include <range/v3/all.hpp>
#include <range/v3/view/map.hpp>
#include <ranges>
#include <tracy/Tracy.hpp>




namespace josh {
namespace {

struct ALight
{
    vec3 color;
};

auto get_active_alight_or_default(const Registry& registry)
    -> ALight
{
    vec3 color = { 0.f, 0.f, 0.f };
    if (const CHandle alight = get_active<AmbientLight>(registry))
        color = alight.get<AmbientLight>().hdr_color();
    return { color };
}

struct DLight
{
    vec3 color;
    vec3 direction;
    bool cast_shadows;
};

auto get_active_dlight_or_default(const Registry& registry)
    -> DLight
{
    vec3 color        = { 0.f, 0.f, 0.f };
    vec3 direction    = { 1.f, 1.f, 1.f };
    bool cast_shadows = false;
    if (const CHandle dlight = get_active<DirectionalLight, MTransform>(registry))
    {
        color        = dlight.get<DirectionalLight>().hdr_color();
        direction    = decompose_rotation(dlight.get<MTransform>()) * -Z;
        cast_shadows = has_tag<ShadowCasting>(dlight);
    }
    return { color, direction, cast_shadows };
}

} // namespace


void DeferredShading::operator()(
    PrimaryContext context)
{
    ZSCGPUN("DeferredShading");
    if (auto* csm = context.belt().try_get<Cascades>())
        update_cascade_buffer(*csm);

    update_point_light_buffers(context.registry());

    switch (mode)
    {
        case Mode::SinglePass: draw_singlepass(context); break;
        case Mode::MultiPass:  draw_multipass(context); break;
    }
}

void DeferredShading::draw_singlepass(
    PrimaryContext context)
{
    const auto& registry  = context.registry();
    auto*       gbuffer   = context.belt().try_get<GBuffer>();
    auto*       point_shadows       = context.belt().try_get<PointShadows>();
    auto*       cascades  = context.belt().try_get<Cascades>();
    auto*       aobuffers = context.belt().try_get<AOBuffers>();

    if (not gbuffer) return;
    // TODO: Could these be optional?
    if (not point_shadows or not cascades) return;

    const BindGuard bcam = context.bind_camera_ubo();

    const RawProgram<> sp = sp_singlepass_.get();

    // GBuffer.
    const MultibindGuard bound_gbuffer = {
        gbuffer->depth_texture()   .bind_to_texture_unit(0),
        gbuffer->normals_texture() .bind_to_texture_unit(1),
        gbuffer->albedo_texture()  .bind_to_texture_unit(2),
        gbuffer->specular_texture().bind_to_texture_unit(3),
        _target_sampler->bind_to_texture_unit(0),
        _target_sampler->bind_to_texture_unit(1),
        _target_sampler->bind_to_texture_unit(2),
        _target_sampler->bind_to_texture_unit(3),
    };

    sp.uniform("gbuffer.tex_depth",    0);
    sp.uniform("gbuffer.tex_normals",  1);
    sp.uniform("gbuffer.tex_albedo",   2);
    sp.uniform("gbuffer.tex_specular", 3);

    // AO.
    if (aobuffers)
    {
        aobuffers->occlusion_texture().bind_to_texture_unit(5);
        auto _ = _ao_sampler->bind_to_texture_unit(5);
        sp.uniform("tex_ambient_occlusion",   5);
        sp.uniform("use_ambient_occlusion",   use_ambient_occlusion);
        sp.uniform("ambient_occlusion_power", ambient_occlusion_power);
    }
    else
    {
        sp.uniform("use_ambient_occlusion", false);
    }

    // Ambient light.
    {
        const auto [color] = get_active_alight_or_default(registry);
        sp.uniform("alight.color", color);
    }

    // Directional light.
    {
        const auto [color, direction, cast_shadows] = get_active_dlight_or_default(registry);
        sp.uniform("dlight.color",        color);
        sp.uniform("dlight.direction",    direction);
        sp.uniform("dlight_cast_shadows", cast_shadows);
    }

    // Directional shadows.
    cascades->maps.textures().bind_to_texture_unit(4);
    const BindGuard bound_csm_sampler = _csm_sampler->bind_to_texture_unit(4);
    sp.uniform("csm_maps",                       4);
    sp.uniform("csm_params.base_bias_tx",        dir_params.base_bias_tx);
    const float blend_size_best_tx =
        cascades->blend_possible ? cascades->blend_max_size_inner_tx : 0.f;
    sp.uniform("csm_params.blend_size_best_tx",  blend_size_best_tx);
    sp.uniform("csm_params.pcf_extent",          dir_params.pcf_extent);
    sp.uniform("csm_params.pcf_offset_inner_tx", dir_params.pcf_offset);
    csm_views_buf_.bind_to_ssbo_index(3);

    // Point lights.
    sp.uniform("plight_fade_start_fraction",  plight_fade_start_fraction);
    plights_with_shadow_buf_.bind_to_ssbo_index(1);
    plights_no_shadow_buf_  .bind_to_ssbo_index(2);

    // Point light shadows.
    point_shadows->maps.cubemaps().bind_to_texture_unit(6);
    BindGuard bound_psm_sampler = _psm_sampler->bind_to_texture_unit(6);
    sp.uniform("psm_maps",               6);
    sp.uniform("psm_params.bias_bounds", point_params.bias_bounds);
    sp.uniform("psm_params.pcf_extent",  point_params.pcf_extent);
    sp.uniform("psm_params.pcf_offset",  point_params.pcf_offset);

    glapi::set_viewport({ {}, context.main_resolution() });
    glapi::disable(Capability::DepthTesting);
    context.bind_back_and([&](auto bfb)
    {
        const BindGuard bsp = sp.use();
        context.primitives().quad_mesh().draw(bsp, bfb);
    });
    glapi::enable(Capability::DepthTesting);

    // The depth buffer is probably shared between the GBuffer
    // and the main framebuffer.
    //
    // This is okay if the deferred shading algorithm does not depend
    // on the depth value. That is, if you need to isolate the
    // depth that was drawn only in deferred passes, then you might
    // have to do just that. And then do some kind of depth blending.
}


/*
TODO: This entire approach is not really viable due to the enormous
bandwidth overhead it creates.

In singlepass we sample the GBuffer once per fragment, and just
iterate over the list of point lights in the scene to compute irradiance.

In multipass we instance draw light volumes (spheres) for the point lights,
which gives us decent frustum and occlusion culling per-light, at the cost
of re-sampling the GBuffer PER-LIGHT SOURCE. If you have N lights illuminating
a single fragment on the screen, then you are doing N-1 redundant samples
of the GBuffer.

So in case there are many lights *on the screen*, multipass is bandwidth heavy
and will likely be much slower that singlepass (especially on my poor iGPU).

In case where most of the lights are *off the screen*, multipass is probably faster
due to culling, although even singlepass can be accompanied by at least basic
frustim culling on the CPU.

One way to reduce the bandwidth a little is to store the irradiance of each fragment
in an intermediate buffer, because computing that only requires normals and roughness
of the surface, and not albedo. But this is not really the solution.

The much more viable solution is to go full in on clustered shading instead,
which has the same bandwidth requirements as singlepass - each fragment samples GBuffer once,
but selectively culls the light volumes per-cluster. Not sure about occlusion culling there though.

Either way, I'm leaving this implementation here for now,
so that it could be used as a stepping stone / testbed for other stuff.
*/
void DeferredShading::draw_multipass(
    PrimaryContext context)
{
    const auto& registry      = context.registry();
    auto*       gbuffer       = context.belt().try_get<GBuffer>();
    auto*       point_shadows = context.belt().try_get<PointShadows>();
    auto*       cascades      = context.belt().try_get<Cascades>();
    auto*       aobuffers     = context.belt().try_get<AOBuffers>();

    if (not gbuffer) return;
    // TODO: Could these be optional?
    if (not point_shadows or not cascades) return;

    glapi::set_viewport({ {}, context.main_resolution() });

    const BindGuard bcam = context.bind_camera_ubo();
    const MultibindGuard bound_gbuffer = {
        gbuffer->depth_texture()   .bind_to_texture_unit(0),
        gbuffer->normals_texture() .bind_to_texture_unit(1),
        gbuffer->albedo_texture()  .bind_to_texture_unit(2),
        gbuffer->specular_texture().bind_to_texture_unit(3),
        _target_sampler->bind_to_texture_unit(0),
        _target_sampler->bind_to_texture_unit(1),
        _target_sampler->bind_to_texture_unit(2),
        _target_sampler->bind_to_texture_unit(3),
    };

    const auto set_gbuffer_uniforms = [&](RawProgram<> sp)
    {
        sp.uniform("gbuffer.tex_depth",    0);
        sp.uniform("gbuffer.tex_normals",  1);
        sp.uniform("gbuffer.tex_albedo",   2);
        sp.uniform("gbuffer.tex_specular", 3);
    };

    // Ambient + Directional Light Pass.
    {
        const RawProgram<> sp = sp_pass_ambi_dir_.get();
        const BindGuard bsp = sp.use();

        set_gbuffer_uniforms(sp);

        // Ambient Light.
        {
            const auto [color] = get_active_alight_or_default(registry);
            sp.uniform("alight.color", color);
        }

        // Ambient Occlusion.
        if (aobuffers)
        {
            aobuffers->occlusion_texture().bind_to_texture_unit(4);
            const BindGuard bound_sampler = _ao_sampler->bind_to_texture_unit(4);
            sp.uniform("use_ambient_occlusion",    use_ambient_occlusion);
            sp.uniform("tex_ambient_occlusion",    4);
            sp.uniform("ambient_occlusion_power",  ambient_occlusion_power);
        }
        else
        {
            sp.uniform("use_ambient_occlusion", false);
        }

        // Directional Light.
        {
            const auto [color, direction, cast_shadows] = get_active_dlight_or_default(registry);
            sp.uniform("dlight.color",        color);
            sp.uniform("dlight.direction",    direction);
            sp.uniform("dlight_cast_shadows", cast_shadows);
        }

        // CSM.
        cascades->maps.textures().bind_to_texture_unit(5);
        BindGuard bound_csm_sampler = _csm_sampler->bind_to_texture_unit(5);
        sp.uniform("csm_maps",                       5);
        sp.uniform("csm_params.base_bias_tx",        dir_params.base_bias_tx);
        const float blend_size_best_tx =
            cascades->blend_possible ? cascades->blend_max_size_inner_tx : 0.f;
        sp.uniform("csm_params.blend_size_best_tx",  blend_size_best_tx);
        sp.uniform("csm_params.pcf_extent",          dir_params.pcf_extent);
        sp.uniform("csm_params.pcf_offset_inner_tx", dir_params.pcf_offset);
        csm_views_buf_.bind_to_ssbo_index(0);

        glapi::disable(Capability::DepthTesting);
        context.bind_back_and([&](auto bfb)
        {
            context.primitives().quad_mesh().draw(bsp, bfb);
        });
        glapi::enable(Capability::DepthTesting);
    }

    const auto instance_draw_plight_spheres = [&](
        usize instance_count,
        auto  bsp)
    {
        context.bind_back_and([&](auto bfb)
        {
            glapi::enable(Capability::DepthTesting);
            glapi::set_depth_test_condition(CompareOp::Greater);
            glapi::set_depth_mask(false);

            glapi::enable(Capability::FaceCulling);
            glapi::set_face_culling_target(Faces::Front);

            glapi::enable(Capability::Blending);
            glapi::set_blend_factors(BlendFactor::One, BlendFactor::One);


            const Mesh& mesh = context.primitives().sphere_mesh();
            const BindGuard bva = mesh.vertex_array().bind();

            glapi::draw_elements_instanced(
                bva, bsp, bfb,
                instance_count,
                mesh.primitive_type(),
                mesh.element_type(),
                mesh.element_offset_bytes(),
                mesh.num_elements()
            );


            glapi::disable(Capability::Blending);

            glapi::set_face_culling_target(Faces::Back);
            glapi::disable(Capability::FaceCulling);

            glapi::set_depth_test_condition(CompareOp::Less);
            glapi::set_depth_mask(true);
            glapi::enable(Capability::DepthTesting);
        });
    };

    // Point Lights No Shadows Pass.
    if (plights_no_shadow_buf_.num_staged() != 0)
    {
        const RawProgram<> sp  = sp_pass_plight_no_shadow_.get();
        const BindGuard    bsp = sp.use();

        set_gbuffer_uniforms(sp);
        sp.uniform("plight_fade_start_fraction", plight_fade_start_fraction);
        plights_no_shadow_buf_.bind_to_ssbo_index(0);

        instance_draw_plight_spheres(plights_no_shadow_buf_.num_staged(), bsp.token());
    }

    // Point Lights With Shadows Pass.
    if (plights_with_shadow_buf_.num_staged() != 0)
    {
        const RawProgram<> sp = sp_pass_plight_with_shadow_.get();
        const BindGuard bsp = sp.use();

        set_gbuffer_uniforms(sp);
        sp.uniform("plight_fade_start_fraction", plight_fade_start_fraction);

        plights_with_shadow_buf_.bind_to_ssbo_index(0);

        // Point Shadows.
        point_shadows->maps.cubemaps().bind_to_texture_unit(4);
        const BindGuard bound_psm_sampler = _psm_sampler->bind_to_texture_unit(4);
        sp.uniform("psm_maps",               4);
        sp.uniform("psm_params.bias_bounds", point_params.bias_bounds);
        sp.uniform("psm_params.pcf_extent",  point_params.pcf_extent);
        sp.uniform("psm_params.pcf_offset",  point_params.pcf_offset);

        instance_draw_plight_spheres(plights_with_shadow_buf_.num_staged(), bsp.token());
    }
}

void DeferredShading::update_cascade_buffer(const Cascades& csm)
{
    ZoneScoped;
    csm_views_buf_.restage(csm.views | transform(CascadeViewGPU::create_from));
}

void DeferredShading::update_point_light_buffers(const Registry& registry)
{
    ZoneScoped;
    // TODO: Uhh, how do I know that the order of lights in the view is the same as the
    // order of shadow cubemaps in `input_psm_->maps`?
    auto plights_with_shadow_view = registry.view<Visible, ShadowCasting, PointLight, MTransform, BoundingSphere>();
    auto plights_no_shadow_view   = registry.view<Visible, PointLight, MTransform, BoundingSphere>(entt::exclude<ShadowCasting>);

    // From tuple<MTransform, PointLight, BoundingSphere> to combined GPU-layout structure.
    const auto repack = [](auto tuple)
    {
        const auto [_, plight, mtf, sphere] = tuple;
        return PointLightBoundedGPU{
            .color    = plight.hdr_color(),
            .position = mtf.decompose_position(),
            .radius   = sphere.radius,
        };
    };

    plights_with_shadow_buf_.restage(plights_with_shadow_view.each() | transform(repack));
    plights_no_shadow_buf_  .restage(plights_no_shadow_view.each()   | transform(repack));
}


} // namespace josh
