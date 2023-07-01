#include "DeferredShadingStage.hpp"
#include "GLShaders.hpp"
#include "RenderComponents.hpp"
#include "RenderEngine.hpp"
#include <entt/entity/registry.hpp>
#include <glbinding/gl/gl.h>
#include <imgui.h>
#include <range/v3/all.hpp>


using namespace gl;

namespace learn {



void DeferredShadingStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    update_point_light_buffers(registry);


    sp_.use().and_then([&, this](ActiveShaderProgram& ashp) {

        gbuffer_->position_target().bind_to_unit_index(0);
        gbuffer_->normals_target().bind_to_unit_index(1);
        gbuffer_->albedo_spec_target().bind_to_unit_index(2);

        ashp.uniform("tex_position_draw", 0)
            .uniform("tex_normals", 1)
            .uniform("tex_albedo_spec", 2);

        for (auto [_, ambi]
            : registry.view<light::Ambient>().each())
        {
            ashp.uniform("ambient_light.color", ambi.color);
        }

        for (auto [e, dir]
            : registry.view<light::Directional>().each())
        {
            ashp.uniform("dir_light.color",        dir.color)
                .uniform("dir_light.direction",    dir.direction)
                .uniform("dir_shadow.do_cast",     registry.all_of<components::ShadowCasting>(e));
        }

        shadow_info_->dir_light_map.depth_target().bind_to_unit_index(3);
        ashp.uniform("dir_shadow.map", 3)
            .uniform("dir_shadow.bias_bounds", dir_params.bias_bounds)
            .uniform("dir_shadow.projection_view", shadow_info_->dir_light_projection_view);



        shadow_info_->point_light_maps.depth_taget().bind_to_unit_index(4);
        ashp.uniform("point_shadow.maps", 4)
            .uniform("point_shadow.bias_bounds", point_params.bias_bounds)
            .uniform("point_shadow.z_far", shadow_info_->point_params.z_near_far[1]);


        ashp.uniform("cam_pos", engine.camera().get_pos());


        engine.draw([&, this] {
            glDisable(GL_DEPTH_TEST);
            quad_renderer_.draw();
            glEnable(GL_DEPTH_TEST);
        });

        // The depth buffer is probably shared between the GBuffer
        // and the main framebuffer.
        //
        // This is okay if the deferred shading algorithm does not depend
        // on the depth value. That is, if you need to isolate the
        // depth that was drawn only in deferred passes, then you might
        // have to do just that. And then do some kind of depth blending.

    });



}




void DeferredShadingStage::update_point_light_buffers(
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









void DeferredShadingStageImGuiHook::operator()() {


    if (ImGui::TreeNode("Point Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Directional Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.dir_params.bias_bounds),
            0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


}



} // namespace learn
