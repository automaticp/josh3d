#include "PointShadowMapping.hpp"
#include "GLAPIBinding.hpp"
#include "GLProgram.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "tags/ShadowCasting.hpp"
#include "tags/AlphaTested.hpp"
#include "components/Materials.hpp"
#include "ECSHelpers.hpp"
#include "Transform.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>


using namespace gl;


namespace josh::stages::primary {


void PointShadowMapping::operator()(
    RenderEnginePrimaryInterface& engine)
{
    resize_cubemap_array_storage_if_needed(engine.registry());

    map_point_shadows(engine);

    // TODO: Wrong responisbility.
    glapi::set_viewport({ {}, engine.main_resolution() });
}




void PointShadowMapping::resize_cubemap_array_storage_if_needed(
    const entt::registry& registry)
{

    auto plights_with_shadow =
        registry.view<light::Point, tags::ShadowCasting>();

    // This technically makes a redundant iteration over the view
    // because getting the size from a view is an O(n) operation.
    //
    // The reality, however is that the number of point lights
    // with shadows in your scene, is not likely to be more than
    // ~10, and even then you're probably already pushing it too far.
    //
    // You could do a stupid thing and use a size_hint() of a view,
    // which is O(1), but then you'd be severly overestimating
    // the number of actual point lights in the scene, and with that,
    // the number of cubemaps needed to be allocated. Given that
    // a single depth cubemap is actually really big in memory,
    // asking for more than you need is a truly bad idea.

    GLsizei new_size = GLsizei(calculate_view_size(plights_with_shadow));

    auto& maps = output_->point_shadow_maps_tgt;

    maps.resize(new_size);
}





void PointShadowMapping::map_point_shadows(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    auto& maps = output_->point_shadow_maps_tgt;


    if (maps.num_array_elements() == 0) { return; }



    glapi::set_viewport({ {}, maps.resolution() });


    auto plights_with_shadows_view =
        registry.view<light::Point, tags::ShadowCasting>();


    auto bound_fbo = maps.bind_draw();

    glapi::clear_depth_buffer(bound_fbo, 1.f);


    auto set_per_light_uniforms = [&, this](
        dsa::RawProgram<> sp,
        const glm::vec3&  pos,
        GLint             cubemap_id)
    {
        glm::mat4 projection = glm::perspective(
            glm::half_pi<float>(), 1.0f,
            output_->z_near_far[0], output_->z_near_far[1]
        );

        const auto& basis = globals::basis;

        const glm::mat4 views[6]{
            glm::lookAt(pos, pos + basis.x(), -basis.y()),
            glm::lookAt(pos, pos - basis.x(), -basis.y()),
            glm::lookAt(pos, pos + basis.y(),  basis.z()),
            glm::lookAt(pos, pos - basis.y(), -basis.z()),
            glm::lookAt(pos, pos + basis.z(), -basis.y()),
            glm::lookAt(pos, pos - basis.z(), -basis.y()),
        };

        Location views_loc = sp.get_uniform_location("views");
        sp.set_uniform_mat4v(views_loc, 6, false, glm::value_ptr(views[0]));

        sp.uniform("projection", projection);
        sp.uniform("cubemap_id", cubemap_id);
        sp.uniform("z_far",      output_->z_near_far[1]);

    };



    {
        auto bound_program = sp_with_alpha->use();

        for (GLint cubemap_id{ 0 };
            auto [_, plight] : plights_with_shadows_view.each())
        {
            set_per_light_uniforms(sp_with_alpha, plight.position, cubemap_id);
            draw_all_world_geometry_with_alpha_test(bound_program, bound_fbo, registry);

            ++cubemap_id;
        }

        bound_program.unbind();
    }


    {
        auto bound_program = sp_no_alpha->use();

        for (GLint cubemap_id{ 0 };
            auto [_, plight] : plights_with_shadows_view.each())
        {
            set_per_light_uniforms(sp_no_alpha, plight.position, cubemap_id);
            draw_all_world_geometry_no_alpha_test(bound_program, bound_fbo, registry);

            ++cubemap_id;
        }

        bound_program.unbind();
    }


    bound_fbo.unbind();
}




void PointShadowMapping::draw_all_world_geometry_no_alpha_test(
    BindToken<Binding::Program>         bound_program,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry)
{
    // Assumes that projection and view are already set.

    auto draw_from_view = [&](auto view) {
        for (auto [entity, world_mtf, mesh] : view.each()) {
            sp_no_alpha->uniform("model", world_mtf.model());
            mesh.draw(bound_program, bound_fbo);
        }
    };

    // You could have no AT requested, or you could have an AT flag,
    // but no diffuse material to sample from.
    //
    // Both ignore Alpha-Testing.

    draw_from_view(
        registry.view<MTransform, Mesh>(entt::exclude<tags::AlphaTested>)
    );

    draw_from_view(
        registry.view<MTransform, Mesh, tags::AlphaTested>(entt::exclude<components::MaterialDiffuse>)
    );

}


void PointShadowMapping::draw_all_world_geometry_with_alpha_test(
    BindToken<Binding::Program>         bound_program,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry)
{
    // Assumes that projection and view are already set.

    sp_with_alpha->uniform("material.diffuse", 0);

    auto meshes_with_alpha_view =
        registry.view<MTransform, Mesh, components::MaterialDiffuse, tags::AlphaTested>();

    for (auto [entity, world_mtf, mesh, diffuse]
        : meshes_with_alpha_view.each())
    {
        (*diffuse.diffuse)->bind_to_texture_unit(0);
        sp_with_alpha->uniform("model", world_mtf.model());
        mesh.draw(bound_program, bound_fbo);
    }

}


} // namespace josh::stages::primary
