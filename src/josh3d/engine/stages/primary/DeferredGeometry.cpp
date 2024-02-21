#include "DeferredGeometry.hpp"
#include "GLMutability.hpp"
#include "GLShaders.hpp"
#include "components/ChildMesh.hpp"
#include "components/Materials.hpp"
#include "tags/Culled.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include "DefaultResources.hpp"
#include <entt/core/type_traits.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>


using namespace gl;

namespace josh::stages::primary {


void DeferredGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    const auto projection = engine.camera().projection_mat();
    const auto view = engine.camera().view_mat();

    // Exclude to not draw the same meshes twice.

    auto material_ds_view = registry.view<MTransform, Mesh>(entt::exclude<components::MaterialNormal, tags::Culled>);
    auto material_dsn_view = registry.view<MTransform, Mesh, components::MaterialNormal>(entt::exclude<tags::Culled>);

    // TODO: Mutual exclusions like these are generally
    // uncomfortable to do in EnTT. Is there a better way?


    const auto apply_ds_materials = [&](entt::entity e, ActiveShaderProgram<GLMutable>& ashp) {

        if (auto mat_d = registry.try_get<components::MaterialDiffuse>(e); mat_d) {
            mat_d->diffuse->bind_to_unit_index(0);
        } else {
            globals::default_diffuse_texture().bind_to_unit_index(0);
        }

        if (auto mat_s = registry.try_get<components::MaterialSpecular>(e); mat_s) {
            mat_s->specular->bind_to_unit_index(1);
            ashp.uniform("material.shininess", mat_s->shininess);
        } else {
            globals::default_specular_texture().bind_to_unit_index(1);
            ashp.uniform("material.shininess", 128.f);
        }

    };


    gbuffer_->bind_draw().and_then([&, this] {

        // FIXME: Poor interaction with alpha-testing.
        if (enable_backface_culling) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }

        sp_ds.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("projection", projection)
                .uniform("view", view);

            ashp.uniform("material.diffuse",  0)
                .uniform("material.specular", 1);

            for (auto [entity, world_mtf, mesh]
                : material_ds_view.each())
            {
                ashp.uniform("model",        world_mtf.model())
                    .uniform("normal_model", world_mtf.normal_model())
                    .uniform("object_id",    entt::to_integral(entity));

                apply_ds_materials(entity, ashp);
                mesh.draw();
            }

        });


        sp_dsn.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("projection", projection)
                .uniform("view", view);

            ashp.uniform("material.diffuse",  0)
                .uniform("material.specular", 1)
                .uniform("material.normal",   2);

            for (auto [entity, world_mtf, mesh, mat_normal]
                : material_dsn_view.each())
            {
                ashp.uniform("model",        world_mtf.model())
                    .uniform("normal_model", world_mtf.normal_model())
                    .uniform("object_id",    entt::to_integral(entity));

                apply_ds_materials(entity, ashp);
                mat_normal.normal->bind_to_unit_index(2);
                mesh.draw();
            }

        });

    });
}



} // namespace josh::stages::primary
