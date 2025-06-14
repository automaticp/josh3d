#include "DeferredGeometry.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "GLAPICore.hpp"
#include "GLProgram.hpp"
#include "MeshStorage.hpp"
#include "StaticMesh.hpp"
#include "UniformTraits.hpp"
#include "Materials.hpp"
#include "tags/AlphaTested.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
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
    const auto* mesh_storage = engine.meshes().storage_for<VertexStatic>();
    auto*       gbuffer      = engine.belt().try_get<GBuffer>();

    if (not mesh_storage) return;
    if (not gbuffer)      return;

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    // Exclude to not draw the same meshes twice.

    // TODO: Anyway, I caved in, and we have 4 variations of shaders now...
    // We should probably rework the mesh layout and remove the combination with/without normals.
    auto view_ds_at    = registry.view<Visible, MTransform, StaticMesh, AlphaTested>(entt::exclude<MaterialNormal>);
    auto view_ds_noat  = registry.view<Visible, MTransform, StaticMesh>(entt::exclude<MaterialNormal, AlphaTested>);
    auto view_dsn_at   = registry.view<Visible, MTransform, StaticMesh, MaterialNormal, AlphaTested>();
    auto view_dsn_noat = registry.view<Visible, MTransform, StaticMesh, MaterialNormal>(entt::exclude<AlphaTested>);

    // TODO: Mutual exclusions like these are generally
    // uncomfortable to do in EnTT. Is there a better way?

    auto apply_ds_materials = [&](Entity e, RawProgram<> sp, Location shininess_loc) {

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


    BindGuard bound_fbo = gbuffer->bind_draw();
    BindGuard bound_vao = mesh_storage->vertex_array().bind();


    auto draw_ds = [&](RawProgram<> sp, auto view) {
        BindGuard bound_program = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);

        Location model_loc        = sp.get_uniform_location("model");
        Location normal_model_loc = sp.get_uniform_location("normal_model");
        Location object_id_loc    = sp.get_uniform_location("object_id");
        Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, world_mtf, mesh] : view.each()) {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_ds_materials(entity, sp, shininess_loc);

            draw_one_from_storage(*mesh_storage, bound_vao, bound_program, bound_fbo, mesh.lods.cur());
        }
    };

    auto draw_dsn = [&](RawProgram<> sp, auto view) {
        BindGuard bound_program = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        Location model_loc        = sp.get_uniform_location("model");
        Location normal_model_loc = sp.get_uniform_location("normal_model");
        Location object_id_loc    = sp.get_uniform_location("object_id");
        Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, world_mtf, mesh, mat_normal] : view.each()) {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_ds_materials(entity, sp, shininess_loc);
            auto _ = mat_normal.texture->bind_to_texture_unit(2);

            draw_one_from_storage(*mesh_storage, bound_vao, bound_program, bound_fbo, mesh.lods.cur());
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
