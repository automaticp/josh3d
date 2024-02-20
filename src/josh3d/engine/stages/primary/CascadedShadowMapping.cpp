#include "CascadedShadowMapping.hpp"
#include "GLMutability.hpp"
#include "GLShaders.hpp"
#include "Logging.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/CulledFromCSM.hpp"
#include "components/Materials.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include "ULocation.hpp"
#include "ECSHelpers.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glbinding-aux/Meta.h>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>


using namespace gl;


namespace josh::stages::primary {


void CascadedShadowMapping::operator()(
    RenderEnginePrimaryInterface& engine)
{
    resize_cascade_storage_if_needed();

    map_dir_light_shadow_cascade(engine, engine.registry());

    auto [w, h] = engine.window_size();
    glViewport(0, 0, w, h);
}




void CascadedShadowMapping::resize_cascade_storage_if_needed() {
    auto& maps = output_->dir_shadow_maps_tgt.depth_attachment();

    const size_t new_size = input_->cascades.size();
    const size_t old_size = maps.size().depth;

    if (new_size != old_size) {
        maps.resize(Size3I{ Size2I{ maps.size() }, new_size });
        // FIXME: Is this needed?
        output_->params.resize(new_size);
    }
}




static void draw_all_world_geometry_no_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    auto draw_from_view = [&](auto view) {
        for (auto [entity, transform, mesh] : view.each()) {
            ashp.uniform("model",
                get_full_mesh_mtransform({ registry, entity }, transform.mtransform()).model());
            mesh.draw();
        }
    };

    // You could have no AT requested, or you could have an AT flag,
    // but no diffuse material to sample from.
    //
    // Both ignore Alpha-Testing.

    draw_from_view(
        registry.view<Transform, Mesh>(
            entt::exclude<tags::AlphaTested, tags::CulledFromCSM>
        )
    );

    draw_from_view(
        registry.view<Transform, Mesh, tags::AlphaTested>(
            entt::exclude<components::MaterialDiffuse, tags::CulledFromCSM>
        )
    );

}


static void draw_all_world_geometry_with_alpha_test(
    ActiveShaderProgram<GLMutable>& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    ashp.uniform("material.diffuse", 0);

    auto meshes_with_alpha_view =
        registry.view<Transform, Mesh, components::MaterialDiffuse, tags::AlphaTested>(
            entt::exclude<tags::CulledFromCSM>
        );

    for (auto [entity, transform, mesh, diffuse]
        : meshes_with_alpha_view.each())
    {
        diffuse.diffuse->bind_to_unit_index(0);
        ashp.uniform("model",
            get_full_mesh_mtransform({ registry, entity }, transform.mtransform()).model());
        mesh.draw();
    }

}




void CascadedShadowMapping::map_dir_light_shadow_cascade(
    const RenderEnginePrimaryInterface&,
    const entt::registry& registry)
{



    output_->params.clear();
    for (const auto& cascade : input_->cascades) {

        const glm::mat4& proj = cascade.projection;
        // Knowing the orthographic projection matrix, we can extract the
        // scale of the cascade in world-space/light-view-space.
        float w =  2.f / proj[0][0];
        float h =  2.f / proj[1][1];
        float d = -2.f / proj[2][2];

        // This is used later in the shading stage.
        output_->params.emplace_back(
            CascadeParams{
                .projview = cascade.projection * cascade.view,
                .scale    = { w, h, d },
                .z_split  = cascade.z_split
            }
        );
    }


    auto& csm_target = output_->dir_shadow_maps_tgt;
    auto& maps = csm_target.depth_attachment();

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    if (maps.size().depth == 0) { return; }

    glViewport(0, 0, maps.size().width, maps.size().height);

    csm_target.bind_draw().and_then([&, this] {

        glClear(GL_DEPTH_BUFFER_BIT);

        const auto& cascades = input_->cascades;
        const auto num_cascades = GLint(std::min(cascades.size(), max_cascades_));

        if (cascades.size() > max_cascades_) {
            // FIXME: Messy. Either resize and recompile shaders,
            // or at least build cascades from largest to smallest,
            // so that only the quality would degrade.
            globals::logstream << "WARNING: Number of input cascades "
                << cascades.size() << " exceedes the stage maximum "
                << max_cascades_ << ". Extra cascades will be ignored.";
        }

        auto set_common_uniforms = [&](ActiveShaderProgram<GLMutable>& ashp) {
            ULocation proj_loc = ashp.location_of("projections");
            ULocation view_loc = ashp.location_of("views");

            for (GLint cascade_id{ 0 }; cascade_id < num_cascades; ++cascade_id) {
                ashp.uniform(proj_loc + cascade_id, cascades[cascade_id].projection)
                    .uniform(view_loc + cascade_id, cascades[cascade_id].view);
            }
            ashp.uniform("num_cascades", num_cascades);
        };



        sp_with_alpha_.use()
            .and_then([&](ActiveShaderProgram<GLMutable>& ashp) {
                set_common_uniforms(ashp);
                draw_all_world_geometry_with_alpha_test(ashp, registry);
            });

        sp_no_alpha_.use()
            .and_then([&](ActiveShaderProgram<GLMutable>& ashp) {
                set_common_uniforms(ashp);
                draw_all_world_geometry_no_alpha_test(ashp, registry);
            });


    })
    .unbind();

}




} // namespace josh::stages::primary
