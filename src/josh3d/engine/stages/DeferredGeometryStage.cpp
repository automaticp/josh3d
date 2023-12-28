#include "DeferredGeometryStage.hpp"
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

namespace josh {


void DeferredGeometryStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    const auto projection = engine.camera().projection_mat();
    const auto view = engine.camera().view_mat();

    const auto get_mtransform = [&](entt::entity e, const Transform& transform)
        -> MTransform
    {
        if (auto as_child = registry.try_get<components::ChildMesh>(e); as_child) {
            return registry.get<Transform>(as_child->parent).mtransform() * transform.mtransform();
        } else {
            return transform.mtransform();
        }
    };

    // Exclude to not draw the same meshes twice.

    auto material_ds_view = registry.view<Transform, Mesh>(entt::exclude<components::MaterialNormal, tags::Culled>);
    auto material_dsn_view = registry.view<Transform, Mesh, components::MaterialNormal>(entt::exclude<tags::Culled>);

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

        sp_ds.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("projection", projection)
                .uniform("view", view);

            ashp.uniform("material.diffuse",  0)
                .uniform("material.specular", 1);

            for (auto [entity, transform, mesh]
                : material_ds_view.each())
            {
                auto model_transform = get_mtransform(entity, transform);
                ashp.uniform("model",        model_transform.model())
                    .uniform("normal_model", model_transform.normal_model())
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

            for (auto [entity, transform, mesh, mat_normal]
                : material_dsn_view.each())
            {
                auto model_transform = get_mtransform(entity, transform);
                ashp.uniform("model",        model_transform.model())
                    .uniform("normal_model", model_transform.normal_model())
                    .uniform("object_id",    entt::to_integral(entity));

                apply_ds_materials(entity, ashp);
                mat_normal.normal->bind_to_unit_index(2);
                mesh.draw();
            }

        });

    });
}



} // namespace josh
