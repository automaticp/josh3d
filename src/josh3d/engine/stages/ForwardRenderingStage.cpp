#include "ForwardRenderingStage.hpp"
#include "GLShaders.hpp"
#include "LightCasters.hpp"
#include "RenderComponents.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>
#include <range/v3/all.hpp>


using namespace gl;


namespace josh {



void ForwardRenderingStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{
    update_point_light_buffers(registry);

    auto [w, h] = engine.window_size();
    glViewport(0, 0, w, h);

    engine.draw([&, this] { draw_scene(engine, registry); });
}




void ForwardRenderingStage::update_point_light_buffers(
    const entt::registry& registry)
{

    auto plights_with_shadow_view =
        registry.view<light::Point, components::ShadowCasting>();

    plights_with_shadows_ssbo_.bind().update(
        plights_with_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<light::Point>(e);
        })
    );

    auto plights_no_shadow_view =
        registry.view<light::Point>(entt::exclude<components::ShadowCasting>);

    plights_no_shadows_ssbo_.bind().update(
        plights_no_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<light::Point>(e);
        })
    );

}




void ForwardRenderingStage::draw_scene(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    sp_.use().and_then([&, this](ActiveShaderProgram& ashp) {

        ashp.uniform("projection",
            engine.camera().perspective_projection_mat(
                engine.window_size().aspect_ratio()
            )
        );
        ashp.uniform("view", engine.camera().view_mat())
            .uniform("cam_pos", engine.camera().get_pos());


        // Ambient light.
        for (auto [_, ambi]
            : registry.view<light::Ambient>().each())
        {
            // Yes, it's a loop. No, it doesn't work if there's
            // more or less than one iteration. This is just
            // a shortest way to unpack a view.
            ashp.uniform("ambient_light.color", ambi.color);
        }


        // Directional light.
        for (auto [e, dir]
            : registry.view<light::Directional>().each())
        {
            ashp.uniform("dir_light.color",        dir.color)
                .uniform("dir_light.direction",    dir.direction)
                .uniform("dir_light_cast_shadows", registry.all_of<components::ShadowCasting>(e));
        }
        ashp.uniform("dir_light_pv",           shadow_info_->dir_light_projection_view)
            .uniform("dir_shadow_bias_bounds", dir_params.bias_bounds)
            .uniform("dir_light_pcf_samples",  dir_params.pcf_samples);

        ashp.uniform("dir_light_shadow_map", 2);
        shadow_info_->dir_light_map.depth_target().bind_to_unit_index(2);


        // Point lights.

        // Point light properties are sent through SSBOs.
        // Send the depth cubemap array for point light shadows.
        ashp.uniform("point_light_shadow_maps", 3);
        shadow_info_->point_light_maps.depth_taget().bind_to_unit_index(3);

        // Extra settings for point light shadows.
        ashp.uniform("point_light_z_far",        shadow_info_->point_params.z_near_far.y)
            .uniform("point_shadow_bias_bounds", point_params.bias_bounds)
            .uniform("point_light_pcf_samples",  point_params.pcf_samples)
            .uniform("point_light_pcf_offset",   point_params.pcf_offset)
            .uniform("point_light_use_fixed_pcf_samples", point_params.use_fixed_pcf_samples);


        // Now for the actual meshes.
        for (auto [e, transform, mesh, material]
            : registry.view<Transform, Mesh, MaterialDS>().each())
        {
            const components::ChildMesh* as_child = registry.try_get<components::ChildMesh>(e);

            auto model_transform = [&]() -> MTransform {
                if (as_child) {
                    return registry.get<Transform>(as_child->parent).mtransform() *
                        transform.mtransform();
                } else {
                    return transform.mtransform();
                }
            }();

            ashp.uniform("model",        model_transform.model())
                .uniform("normal_model", model_transform.normal_model());

            material.apply(ashp);
            mesh.draw();
            // model->draw(ashp);
        }


    });

}





} // namespace josh
