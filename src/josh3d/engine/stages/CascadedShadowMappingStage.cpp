#include "CascadedShadowMappingStage.hpp"
#include "GLShaders.hpp"
#include "GlobalsUtil.hpp"
#include "RenderComponents.hpp"
#include "Transform.hpp"
#include "ULocation.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glbinding-aux/Meta.h>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>


using namespace gl;


namespace josh {


void CascadedShadowMappingStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{
    resize_cascade_storage_if_needed();

    map_dir_light_shadow_cascade(engine, registry);

    auto [w, h] = engine.window_size();
    glViewport(0, 0, w, h);
}




void CascadedShadowMappingStage::resize_cascade_storage_if_needed() {
    const size_t new_size = input_->cascades.size();
    const size_t old_size = output_->dir_shadow_maps.layers();

    if (new_size != old_size) {
        auto& maps = output_->dir_shadow_maps;

        maps.reset_size(
            maps.width(), maps.height(), GLsizei(new_size)
        );
        // FIXME: Is this needed?
        output_->params.resize(new_size);
    }
}




static MTransform get_full_mtransform(
    entt::const_handle handle, const Transform& transform)
{
    if (auto as_child = handle.try_get<components::ChildMesh>(); as_child) {
        return handle.registry()->get<Transform>(as_child->parent).mtransform() *
            transform.mtransform();
    } else {
        return transform.mtransform();
    }
}


static void draw_all_world_geometry_no_alpha_test(
    ActiveShaderProgram& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    auto meshes_no_alpha_view =
        registry.view<Transform, Mesh>(
            entt::exclude<tags::AlphaTested, tags::CulledFromCascadedShadowMapping>
        );

    for (auto [entity, transform, mesh]
        : meshes_no_alpha_view.each())
    {
        ashp.uniform("model",
            get_full_mtransform({ registry, entity }, transform).model());
        mesh.draw();
    }
}


static void draw_all_world_geometry_with_alpha_test(
    ActiveShaderProgram& ashp, const entt::registry& registry)
{
    // Assumes that projection and view are already set.

    ashp.uniform("material.diffuse", 0);

    auto bind_diffuse_material = [&](entt::entity e) {
        if (auto material =
            registry.try_get<components::MaterialDiffuse>(e))
        {
            material->diffuse->bind_to_unit_index(0);
        } else {
            globals::default_diffuse_texture->bind_to_unit_index(0);
        }
    };

    auto meshes_with_alpha_view =
        registry.view<Transform, Mesh, tags::AlphaTested>(
            entt::exclude<tags::CulledFromCascadedShadowMapping>
        );

    for (auto [entity, transform, mesh]
        : meshes_with_alpha_view.each())
    {
        bind_diffuse_material(entity);
        ashp.uniform("model",
            get_full_mtransform({ registry, entity }, transform).model());
        mesh.draw();
    }

}




void CascadedShadowMappingStage::map_dir_light_shadow_cascade(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{


    output_->params.clear();
    for (const auto& cascade : input_->cascades) {
        output_->params.emplace_back(
            CascadeParams{
                .projview = cascade.projection * cascade.view,
                .z_split = cascade.z_split
            }
        );
    }


    auto& maps = output_->dir_shadow_maps;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    if (maps.layers() == 0) { return; }

    glViewport(0, 0, maps.width(), maps.height());

    maps.framebuffer().bind_draw().and_then([&, this] {

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

        auto set_common_uniforms = [&](ActiveShaderProgram& ashp) {
            ULocation proj_loc = ashp.location_of("projections");
            ULocation view_loc = ashp.location_of("views");

            for (GLint cascade_id{ 0 }; cascade_id < num_cascades; ++cascade_id) {
                ashp.uniform(proj_loc + cascade_id, cascades[cascade_id].projection)
                    .uniform(view_loc + cascade_id, cascades[cascade_id].view);
            }
            ashp.uniform("num_cascades", num_cascades);
        };



        sp_with_alpha_.use()
            .and_then([&](ActiveShaderProgram& ashp) {
                set_common_uniforms(ashp);
                draw_all_world_geometry_with_alpha_test(ashp, registry);
            });

        sp_no_alpha_.use()
            .and_then([&](ActiveShaderProgram& ashp) {
                set_common_uniforms(ashp);
                draw_all_world_geometry_no_alpha_test(ashp, registry);
            });


    })
    .unbind();

}




} // namespace josh
