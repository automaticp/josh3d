#include "DeferredGeometry.hpp"
#include "GLProgram.hpp"
#include "UniformTraits.hpp"
#include "Materials.hpp"
#include "tags/AlphaTested.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include "DefaultTextures.hpp"
#include "tags/Visible.hpp"
#include <entt/core/type_traits.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>



namespace josh::stages::primary {


void DeferredGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    // Exclude to not draw the same meshes twice.

    // TODO: Anyway, I caved in, and we have 4 variations of shaders now...
    // We should probably rework the mesh layout and remove the combination with/without normals.
    auto view_ds_at    = registry.view<Visible, MTransform, Mesh, AlphaTested>(entt::exclude<MaterialNormal>);
    auto view_ds_noat  = registry.view<Visible, MTransform, Mesh>(entt::exclude<MaterialNormal, AlphaTested>);
    auto view_dsn_at   = registry.view<Visible, MTransform, Mesh, MaterialNormal, AlphaTested>();
    auto view_dsn_noat = registry.view<Visible, MTransform, Mesh, MaterialNormal>(entt::exclude<AlphaTested>);

    // TODO: Mutual exclusions like these are generally
    // uncomfortable to do in EnTT. Is there a better way?

    const auto apply_ds_materials = [&](entt::entity e, RawProgram<> sp, Location shininess_loc) {

        if (auto mat_d = registry.try_get<MaterialDiffuse>(e)) {
            mat_d->texture->bind_to_texture_unit(0);
        } else {
            globals::default_diffuse_texture().bind_to_texture_unit(0);
        }

        if (auto mat_s = registry.try_get<MaterialSpecular>(e)) {
            mat_s->texture->bind_to_texture_unit(1);
            sp.uniform(shininess_loc, mat_s->shininess);
        } else {
            globals::default_specular_texture().bind_to_texture_unit(1);
            sp.uniform(shininess_loc, 128.f);
        }

    };



    BindGuard bound_fbo = gbuffer_->bind_draw();


    auto draw_ds = [&](RawProgram<> sp, auto entt_view) {
        BindGuard bound_program = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);

        Location model_loc        = sp.get_uniform_location("model");
        Location normal_model_loc = sp.get_uniform_location("normal_model");
        Location object_id_loc    = sp.get_uniform_location("object_id");
        Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, world_mtf, mesh] : entt_view.each()) {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_ds_materials(entity, sp, shininess_loc);

            mesh.draw(bound_program, bound_fbo);
        }
    };

    auto draw_dsn = [&](RawProgram<> sp, auto entt_view) {
        BindGuard bound_program = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        Location model_loc        = sp.get_uniform_location("model");
        Location normal_model_loc = sp.get_uniform_location("normal_model");
        Location object_id_loc    = sp.get_uniform_location("object_id");
        Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, world_mtf, mesh, mat_normal] : entt_view.each()) {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_ds_materials(entity, sp, shininess_loc);
            auto _ = mat_normal.texture->bind_to_texture_unit(2);

            mesh.draw(bound_program, bound_fbo);
        }
    };


    // Not Alpha-Tested. Opaque.
    // Can be backface culled.

    if (enable_backface_culling) {
        glapi::enable(Capability::FaceCulling);
    } else {
        glapi::disable(Capability::FaceCulling);
    }

    draw_ds (sp_ds_noat.get(),  view_ds_noat );
    draw_dsn(sp_dsn_noat.get(), view_dsn_noat);


    // Alpha-Tested.
    // No backface culling even if requested.

    glapi::disable(Capability::FaceCulling);

    draw_ds (sp_ds_at.get(),  view_ds_at );
    draw_dsn(sp_dsn_at.get(), view_dsn_at);

}



} // namespace josh::stages::primary
