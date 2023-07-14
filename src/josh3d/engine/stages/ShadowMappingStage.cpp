#include "ShadowMappingStage.hpp"
#include "GLShaders.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "RenderComponents.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "RenderComponents.hpp"
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




void ShadowMappingStage::map_point_light_shadows(
    const RenderEnginePrimaryInterface& /* engine */,
    const entt::registry& registry)
{

    auto plights_with_shadow_view =
        registry.view<const light::Point, const components::ShadowCasting>();

    sp_plight_depth_.use().and_then([&, this](ActiveShaderProgram& ashp) {

        auto& maps = mapping_output_->point_light_maps;
        glViewport(0, 0, maps.width(), maps.height());

        maps.framebuffer().bind_draw().and_then([&, this] {

            GLint cubemap_id{ 0 };

            for (auto [_, plight]
                : plights_with_shadow_view.each())
            {
                if (cubemap_id == 0) /* first time */ {
                    glClear(GL_DEPTH_BUFFER_BIT);
                }

                draw_scene_depth_onto_cubemap(ashp, registry, plight.position, cubemap_id);

                ++cubemap_id;
            }

        })
        .unbind();

    });

}








static void draw_all_world_geometry(ActiveShaderProgram& ashp,
    const entt::registry& registry)
{

    for (auto [_, transform, mesh]
        : registry.view<Transform, Mesh>(entt::exclude<components::ChildMesh>).each())
    {
        ashp.uniform("model", transform.mtransform().model());
        mesh.draw();
    }


    for (auto [_, transform, mesh, as_child]
        : registry.view<Transform, Mesh, components::ChildMesh>().each())
    {
        const Transform& parent_transform =
            registry.get<Transform>(as_child.parent);

        const MTransform full_model_matrix =
            parent_transform.mtransform() * transform.mtransform();

        ashp.uniform("model", full_model_matrix.model());
        mesh.draw();
    }

}








void ShadowMappingStage::draw_scene_depth_onto_cubemap(
    ActiveShaderProgram& ashp, const entt::registry& registry,
    const glm::vec3& position, gl::GLint cubemap_id)
{
    auto& maps = mapping_output_->point_light_maps;

    glm::mat4 projection = glm::perspective(
        glm::radians(90.f),
        static_cast<float>(maps.width()) /
            static_cast<float>(maps.height()),
        point_params().z_near_far.x, point_params().z_near_far.y
    );

    ashp.uniform("projection", projection);

    const auto& basis = globals::basis;

    const std::array<glm::mat4, 6> views{
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

    ashp.uniform("z_far", point_params().z_near_far.y);


    draw_all_world_geometry(ashp, registry);
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


    sp_dir_depth_.use().and_then([&, this] (ActiveShaderProgram& ashp) {

        auto& map = mapping_output_->dir_light_map;
        glViewport(0, 0, map.width(), map.height());

        map.framebuffer().bind_draw().and_then([&, this] {

            glClear(GL_DEPTH_BUFFER_BIT);

            draw_scene_depth_onto_texture(ashp, registry, light_view, light_projection);

        }).unbind();

    });

}




void ShadowMappingStage::draw_scene_depth_onto_texture(
    ActiveShaderProgram& ashp, const entt::registry& registry,
    const glm::mat4& view, const glm::mat4& projection)
{
    ashp.uniform("projection", projection)
        .uniform("view",       view);

    draw_all_world_geometry(ashp, registry);
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
