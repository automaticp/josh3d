#include "SkinnedGeometry.hpp"
#include "DefaultTextures.hpp"
#include "GLAPICore.hpp"
#include "Materials.hpp"
#include "MeshStorage.hpp"
#include "RenderEngine.hpp"
#include "UniformTraits.hpp"
#include "SkinnedMesh.hpp"
#include "VertexSkinned.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/Visible.hpp"
#include "ECS.hpp"


namespace josh {


void SkinnedGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry     = engine.registry();
    const auto* mesh_storage = engine.meshes().storage_for<VertexSkinned>();
    auto*       gbuffer      = engine.belt().try_get<GBuffer>();

    if (not mesh_storage) return;
    if (not gbuffer)      return;

    const BindGuard bva  = mesh_storage->vertex_array().bind();
    const BindGuard bcam = engine.bind_camera_ubo();
    const BindGuard bfb  = gbuffer->bind_draw();

    glapi::set_viewport({ {}, gbuffer->resolution() });

    auto view_opaque  = registry.view<Visible, MTransform, SkinnedMe2h, Pose>(entt::exclude<AlphaTested>);
    auto view_atested = registry.view<Visible, MTransform, SkinnedMe2h, Pose, AlphaTested>();

    const auto apply_materials = [&](Entity e, RawProgram<> sp, Location shininess_loc)
    {
        if (auto* mat_d = registry.try_get<MaterialDiffuse>(e))
        {
            mat_d->texture->bind_to_texture_unit(0);
        }
        else
        {
            globals::default_diffuse_texture().bind_to_texture_unit(0);
        }

        if (auto* mat_s = registry.try_get<MaterialSpecular>(e))
        {
            mat_s->texture->bind_to_texture_unit(1);
            sp.uniform(shininess_loc, mat_s->shininess);
        }
        else
        {
            globals::default_specular_texture().bind_to_texture_unit(1);
            sp.uniform(shininess_loc, 128.f);
        }

        if (auto* mat_n = registry.try_get<MaterialNormal>(e))
        {
            mat_n->texture->bind_to_texture_unit(2);
        }
        else
        {
            globals::default_normal_texture().bind_to_texture_unit(2);
        }
    };

    auto draw_from_view = [&](RawProgram<> sp, auto view)
    {
        const BindGuard bsp = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        const Location model_loc        = sp.get_uniform_location("model");
        const Location normal_model_loc = sp.get_uniform_location("normal_model");
        const Location object_id_loc    = sp.get_uniform_location("object_id");
        const Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, world_mtf, skinned_mesh, pose] : view.each())
        {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_materials(entity, sp, shininess_loc);

            skinning_mats_.restage(pose.skinning_mats);
            skinning_mats_.bind_to_ssbo_index(0);

            // TODO: Could batch if had SkinStorage.
            draw_one_from_storage(*mesh_storage, bva, bsp, bfb, skinned_mesh.lods.cur());
        }
    };

    // Not Alpha-Tested. Opaque.
    // Can be backface culled.
    if (backface_culling)
        glapi::enable(Capability::FaceCulling);
    else
        glapi::disable(Capability::FaceCulling);

    draw_from_view(sp_opaque.get(), view_opaque);

    // Alpha-Tested.
    // No backface culling even if requested.
    glapi::disable(Capability::FaceCulling);
    draw_from_view(sp_atested.get(), view_atested);
}


} // namespace josh
