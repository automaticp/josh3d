#include "DeferredGeometry.hpp"
#include "GLProgram.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "components/Materials.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/Culled.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include <entt/core/type_traits.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>



namespace josh::stages::primary {


void DeferredGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    const auto proj = engine.camera().projection_mat();
    const auto view = engine.camera().view_mat();

    // Exclude to not draw the same meshes twice.

    // TODO: Anyway, I caved in, and we have 4 variations of shaders now...
    // We should probably rework the mesh layout and remove the combination with/withoun normals.
    auto view_ds_at    = registry.view<MTransform, Mesh, tags::AlphaTested>(entt::exclude<components::MaterialNormal, tags::Culled>);
    auto view_ds_noat  = registry.view<MTransform, Mesh>(entt::exclude<components::MaterialNormal, tags::Culled, tags::AlphaTested>);
    auto view_dsn_at   = registry.view<MTransform, Mesh, components::MaterialNormal, tags::AlphaTested>(entt::exclude<tags::Culled>);
    auto view_dsn_noat = registry.view<MTransform, Mesh, components::MaterialNormal>(entt::exclude<tags::Culled, tags::AlphaTested>);

    // TODO: Mutual exclusions like these are generally
    // uncomfortable to do in EnTT. Is there a better way?


    const auto apply_ds_materials = [&](entt::entity e, RawProgram<> sp) {

        if (auto mat_d = registry.try_get<components::MaterialDiffuse>(e)) {
            mat_d->texture->bind_to_texture_unit(0);
        } else {
            engine.primitives().default_diffuse_texture().bind_to_texture_unit(0);
        }

        if (auto mat_s = registry.try_get<components::MaterialSpecular>(e)) {
            mat_s->texture->bind_to_texture_unit(1);
            sp.uniform("material.shininess", mat_s->shininess);
        } else {
            engine.primitives().default_specular_texture().bind_to_texture_unit(1);
            sp.uniform("material.shininess", 128.f);
        }

    };



    auto bound_fbo = gbuffer_->bind_draw();


    auto draw_ds = [&](RawProgram<> sp, auto entt_view) {
        auto bound_program = sp.use();

        sp.uniform("projection", proj);
        sp.uniform("view",       view);

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);

        for (auto [entity, world_mtf, mesh] : entt_view.each()) {
            sp.uniform("model",        world_mtf.model());
            sp.uniform("normal_model", world_mtf.normal_model());
            sp.uniform("object_id",    entt::to_integral(entity));

            apply_ds_materials(entity, sp);

            mesh.draw(bound_program, bound_fbo);
        }

        bound_program.unbind();
    };

    auto draw_dsn = [&](RawProgram<> sp, auto entt_view) {
        auto bound_program = sp.use();

        sp.uniform("projection", proj);
        sp.uniform("view",       view);

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        for (auto [entity, world_mtf, mesh, mat_normal] : entt_view.each()) {
            sp.uniform("model",        world_mtf.model());
            sp.uniform("normal_model", world_mtf.normal_model());
            sp.uniform("object_id",    entt::to_integral(entity));

            apply_ds_materials(entity, sp);
            mat_normal.texture->bind_to_texture_unit(2);

            mesh.draw(bound_program, bound_fbo);
        }

        bound_program.unbind();
    };


    // Not Alpha-Tested. Opaque.
    // Can be backface culled.

    if (enable_backface_culling) {
        glapi::enable(Capability::FaceCulling);
    } else {
        glapi::disable(Capability::FaceCulling);
    }

    draw_ds (sp_ds_noat,  view_ds_noat );
    draw_dsn(sp_dsn_noat, view_dsn_noat);


    // Alpha-Tested.
    // No backface culling even if requested.

    glapi::disable(Capability::FaceCulling);

    draw_ds (sp_ds_at,  view_ds_at );
    draw_dsn(sp_dsn_at, view_dsn_at);



    bound_fbo.unbind();
}



} // namespace josh::stages::primary
