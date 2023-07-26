#include "ShadowMappingStage.hpp"
#include "GLShaders.hpp"
#include "LightCasters.hpp"
#include "RenderComponents.hpp"
#include "Mesh.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/functions.h>
#include <imgui.h>


using namespace gl;


namespace josh {



void ShadowMappingStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    resize_point_light_cubemap_array_if_needed(registry);

    map_point_light_shadows(engine, registry);
    map_dir_light_shadows(engine, registry);

    // TODO: Am I a good citizen now?
    gl::glViewport(0, 0, engine.window_size().width, engine.window_size().height);

}








static size_t calculate_view_size(auto view) noexcept {
    size_t count{ 0 };
    for (auto  _ [[maybe_unused]] : view.each()) { ++count; }
    return count;
}


void ShadowMappingStage::resize_point_light_cubemap_array_if_needed(
    const entt::registry& registry)
{

    auto plights_with_shadow =
        registry.view<light::Point, components::ShadowCasting>();

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

    auto& maps = mapping_output_->point_light_maps;
    size_t old_size = maps.depth();

    if (new_size != old_size) {
        maps.reset_size(
            maps.width(), maps.height(), GLsizei(new_size)
        );
    }

}








static MTransform get_full_mtransform(entt::const_handle handle,
    const Transform& transform)
{
    if (auto as_child = handle.try_get<components::ChildMesh>(); as_child) {
        return handle.registry()->get<Transform>(as_child->parent).mtransform() *
            transform.mtransform();
    } else {
        return transform.mtransform();
    }
}




static void draw_all_world_geometry_with_alpha_test(ActiveShaderProgram& ashp,
    const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    ashp.uniform("material.diffuse", 0);

    // FIXME: To be removed once alpha test filtering is there
    auto bind_diffuse_material = [&](entt::entity e) {
        if (auto material = registry.try_get<components::MaterialDiffuse>(e); material) {
            material->diffuse->bind_to_unit_index(0);
        } else {
            globals::default_diffuse_texture->bind_to_unit_index(0);
        }
    };

    auto meshes_with_alpha_view = registry.view<Transform, Mesh, components::AlphaTested>();

    for (auto [entity, transform, mesh]
        : meshes_with_alpha_view.each())
    {
        bind_diffuse_material(entity);
        ashp.uniform("model", get_full_mtransform({ registry, entity }, transform).model());
        mesh.draw();
    }

}




static void draw_all_world_geometry_no_alpha_test(ActiveShaderProgram& ashp,
    const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    auto meshes_no_alpha_view =
        registry.view<Transform, Mesh>(entt::exclude<components::AlphaTested>);

    for (auto [entity, transform, mesh]
        : meshes_no_alpha_view.each())
    {
        ashp.uniform("model", get_full_mtransform({ registry, entity }, transform).model());
        mesh.draw();
    }

}








static void set_common_point_shadow_uniforms(
    ActiveShaderProgram& ashp, const glm::vec3& position,
    const ShadowMappingStage::PointShadowParams& params,
    GLint cubemap_id)
{

    glm::mat4 projection = glm::perspective(
        glm::radians(90.f), 1.0f,
        params.z_near_far.x, params.z_near_far.y
    );

    ashp.uniform("projection", projection);

    const auto& basis = globals::basis;

    const glm::mat4 views[6]{
        glm::lookAt(position, position + basis.x(), -basis.y()),
        glm::lookAt(position, position - basis.x(), -basis.y()),
        glm::lookAt(position, position + basis.y(),  basis.z()),
        glm::lookAt(position, position - basis.y(), -basis.z()),
        glm::lookAt(position, position + basis.z(), -basis.y()),
        glm::lookAt(position, position - basis.z(), -basis.y()),
    };

    ashp.uniform("views[0]", views[0]);
    ashp.uniform("views[1]", views[1]);
    ashp.uniform("views[2]", views[2]);
    ashp.uniform("views[3]", views[3]);
    ashp.uniform("views[4]", views[4]);
    ashp.uniform("views[5]", views[5]);


    ashp.uniform("cubemap_id", cubemap_id);

    ashp.uniform("z_far", params.z_near_far.y);

}




void ShadowMappingStage::map_point_light_shadows(
    const RenderEnginePrimaryInterface& /* engine */,
    const entt::registry& registry)
{

    auto plights_with_shadow_view =
        registry.view<light::Point, components::ShadowCasting>();

    auto& maps = mapping_output_->point_light_maps;
    glViewport(0, 0, maps.width(), maps.height());


    maps.framebuffer().bind_draw().and_then([&, this] {

        if (maps.depth() /* (aka. cubemap array size) */ != 0) {
            // glClear on an empty array render target will error out.
            glClear(GL_DEPTH_BUFFER_BIT);
        }


        sp_plight_depth_with_alpha_.use().and_then([&, this](ActiveShaderProgram& ashp) {

            for (GLint cubemap_id{ 0 };
                auto [_, plight] : plights_with_shadow_view.each())
            {

                set_common_point_shadow_uniforms(
                    ashp, plight.position, point_params(), cubemap_id
                );

                draw_all_world_geometry_with_alpha_test(ashp, registry);

                ++cubemap_id;
            }

        });


        sp_plight_depth_no_alpha_.use().and_then([&, this](ActiveShaderProgram& ashp) {

            for (GLint cubemap_id{ 0 };
                auto [_, plight] : plights_with_shadow_view.each())
            {

                set_common_point_shadow_uniforms(
                    ashp, plight.position, point_params(), cubemap_id
                );

                draw_all_world_geometry_no_alpha_test(ashp, registry);

                ++cubemap_id;
            }

        });


    })
    .unbind();

}









void ShadowMappingStage::map_dir_light_shadows(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    // Only one directional light supported for shadowing.
    // No idea what happens to you if there's more in the registry.
    entt::entity the_only_light_that_matters =
        registry.view<light::Directional, components::ShadowCasting>().back();

    if (the_only_light_that_matters == entt::null) {
        return;
    }

    const auto& dir_light =
        registry.get<light::Directional>(the_only_light_that_matters);


    glm::mat4 light_projection = glm::ortho(
        -dir_params().projection_scale, dir_params().projection_scale,
        -dir_params().projection_scale, dir_params().projection_scale,
        dir_params().z_near_far.x, dir_params().z_near_far.y
    );

    glm::mat4 light_view = glm::lookAt(
        engine.camera().get_pos()
            - dir_params().cam_offset * glm::normalize(dir_light.direction),
        engine.camera().get_pos(),
        globals::basis.y()
    );

    // Is exported to ShadowMapStorage for reading in later stages.
    mapping_output_->dir_light_projection_view = light_projection * light_view;


    auto& map = mapping_output_->dir_light_map;
    glViewport(0, 0, map.width(), map.height());

    map.framebuffer().bind_draw().and_then([&, this] {

        glClear(GL_DEPTH_BUFFER_BIT);


        sp_dir_depth_with_alpha.use()
            .uniform("projection", light_projection)
            .uniform("view",       light_view)
            .and_then([&](ActiveShaderProgram& ashp) {
                draw_all_world_geometry_with_alpha_test(ashp, registry);
            });


        sp_dir_depth_no_alpha.use()
            .uniform("projection", light_projection)
            .uniform("view",       light_view)
            .and_then([&](ActiveShaderProgram& ashp) {
                draw_all_world_geometry_no_alpha_test(ashp, registry);
            });

    })
    .unbind();

}








void ShadowMappingStage::resize_dir_map(
    gl::GLsizei width, gl::GLsizei height)
{
    mapping_output_->dir_light_map.reset_size(width, height);
}


void ShadowMappingStage::resize_point_maps(
    gl::GLsizei width, gl::GLsizei height)
{
    mapping_output_->point_light_maps.reset_size(
        width, height, mapping_output_->point_light_maps.depth()
    );
}




} // namespace josh