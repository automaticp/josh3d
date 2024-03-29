#include "PointShadowMapping.hpp"
#include "GLMutability.hpp"
#include "tags/ShadowCasting.hpp"
#include "tags/AlphaTested.hpp"
#include "components/Materials.hpp"
#include "ECSHelpers.hpp"
#include "Size.hpp"
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

    auto [w, h] = engine.window_size();
    glViewport(0, 0, w, h);
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

    size_t new_size = calculate_view_size(plights_with_shadow);

    auto& maps = output_->point_shadow_maps_tgt.depth_attachment();
    size_t old_size = maps.size().depth;

    if (new_size != old_size) {
        maps.resize({ Size2I{ maps.size() }, new_size });
    }

}




static void draw_all_world_geometry_with_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry);


static void draw_all_world_geometry_no_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry);




void PointShadowMapping::map_point_shadows(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    auto& maps_tgt = output_->point_shadow_maps_tgt;
    auto& maps = maps_tgt.depth_attachment();

    if (maps.size().depth == 0) { return; }

    glViewport(0, 0, maps.size().width, maps.size().height);

    auto plights_with_shadows_view =
        registry.view<light::Point, tags::ShadowCasting>();


    maps_tgt.bind_draw().and_then([&, this] {

        glClear(GL_DEPTH_BUFFER_BIT);


        auto set_common_uniforms = [&, this](
            ActiveShaderProgram<GLMutable>& ashp,
            const glm::vec3& pos, GLint cubemap_id)
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

            for(GLint i{ 0 }, loc{ ashp.location_of("views") };
                i < 6; ++i, ++loc)
            {
                ashp.uniform(loc, views[i]);
            }

            ashp.uniform("projection", projection)
                .uniform("cubemap_id", cubemap_id)
                .uniform("z_far",      output_->z_near_far[1]);

        };


        sp_with_alpha.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {
            for (GLint cubemap_id{ 0 };
                auto [_, plight] : plights_with_shadows_view.each())
            {
                set_common_uniforms(ashp, plight.position, cubemap_id);
                draw_all_world_geometry_with_alpha_test(ashp, registry);

                ++cubemap_id;
            }
        });


        sp_no_alpha.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {
            for (GLint cubemap_id{ 0 };
                auto [_, plight] : plights_with_shadows_view.each())
            {
                set_common_uniforms(ashp, plight.position, cubemap_id);
                draw_all_world_geometry_no_alpha_test(ashp, registry);

                ++cubemap_id;
            }
        });


    })
    .unbind();

}




static void draw_all_world_geometry_no_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    auto draw_from_view = [&](auto view) {
        for (auto [entity, world_mtf, mesh] : view.each()) {
            ashp.uniform("model", world_mtf.model());
            mesh.draw();
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


static void draw_all_world_geometry_with_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    ashp.uniform("material.diffuse", 0);

    auto meshes_with_alpha_view =
        registry.view<MTransform, Mesh, components::MaterialDiffuse, tags::AlphaTested>();

    for (auto [entity, world_mtf, mesh, diffuse]
        : meshes_with_alpha_view.each())
    {
        diffuse.diffuse->bind_to_unit_index(0);
        ashp.uniform("model", world_mtf.model());
        mesh.draw();
    }

}


} // namespace josh::stages::primary
