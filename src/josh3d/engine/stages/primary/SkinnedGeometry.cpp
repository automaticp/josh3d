#include "SkinnedGeometry.hpp"
#include "DefaultTextures.hpp"
#include "Materials.hpp"
#include "RenderEngine.hpp"
#include "UniformTraits.hpp"
#include "SkinnedMesh.hpp"
#include "VertexSkinned.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/Visible.hpp"


namespace josh::stages::primary {


void SkinnedGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    if (!engine.meshes().has_storage_for<VertexSkinned>()) return;

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();
    BindGuard bound_fbo        = gbuffer_->bind_draw();

    auto view_opaque  = registry.view<Visible, MTransform, SkinnedMesh>(entt::exclude<AlphaTested>);
    auto view_atested = registry.view<Visible, MTransform, SkinnedMesh, AlphaTested>();

    const auto apply_materials = [&](entt::entity e, RawProgram<> sp, Location shininess_loc) {

        if (auto* mat_d = registry.try_get<MaterialDiffuse>(e)) {
            mat_d->texture->bind_to_texture_unit(0);
        } else {
            globals::default_diffuse_texture().bind_to_texture_unit(0);
        }

        if (auto* mat_s = registry.try_get<MaterialSpecular>(e)) {
            mat_s->texture->bind_to_texture_unit(1);
            sp.uniform(shininess_loc, mat_s->shininess);
        } else {
            globals::default_specular_texture().bind_to_texture_unit(1);
            sp.uniform(shininess_loc, 128.f);
        }

        if (auto* mat_n = registry.try_get<MaterialNormal>(e)) {
            mat_n->texture->bind_to_texture_unit(2);
        } else {
            globals::default_normal_texture().bind_to_texture_unit(2);
        }

    };


    auto draw_from_view = [&](RawProgram<> sp, auto entt_view) {
        BindGuard bound_program = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        Location model_loc        = sp.get_uniform_location("model");
        Location normal_model_loc = sp.get_uniform_location("normal_model");
        Location object_id_loc    = sp.get_uniform_location("object_id");
        Location shininess_loc    = sp.get_uniform_location("material.shininess");

        auto& mesh_storage = *engine.meshes().storage_for<VertexSkinned>();
        BindGuard bound_vao = mesh_storage.vertex_array().bind();

        for (auto [entity, world_mtf, skinned_mesh] : entt_view.each()) {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_materials(entity, sp, shininess_loc);

            skinning_mats_.restage(skinned_mesh.pose.skinning_mats);
            BindGuard bound_skin_mats = skinning_mats_.bind_to_ssbo_index(0);

            // TODO: Could batch if had SkinStorage.
            auto [offset_bytes, count, basevert] = mesh_storage.query_one(skinned_mesh.mesh_id);

            glapi::draw_elements_basevertex(
                bound_vao,
                bound_program,
                bound_fbo,
                mesh_storage.primitive_type(),
                mesh_storage.element_type(),
                offset_bytes,
                count,
                basevert
            );
        }
    };


    // Not Alpha-Tested. Opaque.
    // Can be backface culled.
    if (enable_backface_culling) {
        glapi::enable(Capability::FaceCulling);
    } else {
        glapi::disable(Capability::FaceCulling);
    }
    draw_from_view(sp_opaque.get(), view_opaque);

    // Alpha-Tested.
    // No backface culling even if requested.
    glapi::disable(Capability::FaceCulling);
    draw_from_view(sp_atested.get(), view_atested);

}

} // namespace josh::stages::primary
