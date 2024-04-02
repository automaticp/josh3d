#include "CascadedShadowMapping.hpp"
#include "GLAPIBinding.hpp"
#include "GLMutability.hpp"
#include "GLProgram.hpp"
#include "GLShaders.hpp"
#include "Logging.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/CulledFromCSM.hpp"
#include "components/Materials.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include "UniformTraits.hpp"
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

    // TODO: Whose responsibility it is to set the viewport? Not of this stage tbh.
    glapi::set_viewport({ {}, engine.main_resolution() });
}




void CascadedShadowMapping::resize_cascade_storage_if_needed() {
    auto& maps = output_->dir_shadow_maps_tgt;

    const Size2I  new_resolution   = input_->resolution;
    const GLsizei new_num_cascades = GLsizei(input_->cascades.size());

    // Will only resize on mismatch.
    maps.resize(new_resolution, new_num_cascades);
}




void CascadedShadowMapping::draw_all_world_geometry_no_alpha_test(
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry)
{
    // Assumes that projection and view are already set.

    auto bound_program = sp_no_alpha_->use();

    auto draw_from_view = [&, this](auto view) {
        for (auto [entity, world_mtf, mesh] : view.each()) {
            sp_no_alpha_->uniform("model", world_mtf.model());
            mesh.draw(bound_program, bound_fbo);
        }
    };

    // You could have no AT requested, or you could have an AT flag,
    // but no diffuse material to sample from.
    //
    // Both ignore Alpha-Testing.

    draw_from_view(
        registry.view<MTransform, Mesh>(
            entt::exclude<tags::AlphaTested, tags::CulledFromCSM>
        )
    );

    draw_from_view(
        registry.view<MTransform, Mesh, tags::AlphaTested>(
            entt::exclude<components::MaterialDiffuse, tags::CulledFromCSM>
        )
    );

    bound_program.unbind();
}


void CascadedShadowMapping::draw_all_world_geometry_with_alpha_test(
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry)
{
    // Assumes that projection and view are already set.

    auto bound_program = sp_with_alpha_->use();

    sp_with_alpha_->uniform("material.diffuse", 0);

    auto meshes_with_alpha_view =
        registry.view<MTransform, Mesh, components::MaterialDiffuse, tags::AlphaTested>(
            entt::exclude<tags::CulledFromCSM>
        );

    for (auto [entity, world_mtf, mesh, diffuse]
        : meshes_with_alpha_view.each())
    {
        // TODO: Shared<GLUnique<...>> uh-huh, yeah
        // TODO: Why does recursion with "->" not work?
        (*diffuse.diffuse)->bind_to_texture_unit(0);
        sp_with_alpha_->uniform("model", world_mtf.model());
        mesh.draw(bound_program, bound_fbo);
    }

    bound_program.unbind();
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
    const auto& maps = csm_target;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    if (maps.num_array_elements() == 0) { return; }

    glapi::set_viewport({ {}, maps.resolution() });
    glapi::enable(Capability::DepthTesting);

    auto bound_fbo = csm_target.bind_draw();

    glapi::clear_depth_buffer(bound_fbo, 1.f);

    const auto& cascades     = input_->cascades;
    const auto  num_cascades = GLint(std::min(cascades.size(), max_cascades_));

    if (cascades.size() > max_cascades_) {
        // FIXME: Messy. Either resize and recompile shaders,
        // or at least build cascades from largest to smallest,
        // so that only the quality would degrade.
        globals::logstream
            << "WARNING: Number of input cascades " << cascades.size()
            << " exceedes the stage maximum "       << max_cascades_
            << ". Extra cascades will be ignored.";
    }

    auto set_common_uniforms = [&](dsa::RawProgram<> sp) {
        Location proj_loc = sp.get_uniform_location("projections");
        Location view_loc = sp.get_uniform_location("views");

        for (GLsizei cascade_id{ 0 }; cascade_id < num_cascades; ++cascade_id) {
            sp.uniform(Location{ proj_loc + cascade_id }, cascades[cascade_id].projection);
            sp.uniform(Location{ view_loc + cascade_id }, cascades[cascade_id].view);
        }
        sp.uniform("num_cascades", num_cascades);
    };


    set_common_uniforms(sp_with_alpha_);
    draw_all_world_geometry_with_alpha_test(bound_fbo, registry);

    set_common_uniforms(sp_no_alpha_);
    draw_all_world_geometry_no_alpha_test(bound_fbo, registry);


    bound_fbo.unbind();
}




} // namespace josh::stages::primary
