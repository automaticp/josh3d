#include "SkinnedGeometry.hpp"
#include "DefaultTextures.hpp"
#include "DrawHelpers.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "MeshStorage.hpp"
#include "StageContext.hpp"
#include "UniformTraits.hpp"
#include "SkinnedMesh.hpp"
#include "VertexSkinned.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "AlphaTested.hpp"
#include "Visible.hpp"
#include "ECS.hpp"
#include "Tracy.hpp"


namespace josh {


void SkinnedGeometry::operator()(
    PrimaryContext context)
{
    ZSCGPUN("SkinnedGeometry");
    const auto& registry     = context.registry();
    const auto* mesh_storage = context.mesh_registry().storage_for<VertexSkinned>();
    auto*       gbuffer      = context.belt().try_get<GBuffer>();

    if (not mesh_storage) return;
    if (not gbuffer)      return;

    const BindGuard bva  = mesh_storage->vertex_array().bind();
    const BindGuard bcam = context.bind_camera_ubo();
    const BindGuard bfb  = gbuffer->bind_draw();

    glapi::set_viewport({ {}, gbuffer->resolution() });

    // FIXME: Negative filtering. Replace with Opaque tag or Not<AlphaTested> flag.

    auto view_opaque  = registry.view<Visible, MTransform, SkinnedMe2h, Pose>(entt::exclude<AlphaTested>);
    auto view_atested = registry.view<Visible, MTransform, SkinnedMe2h, Pose, AlphaTested>();

    const Array<u32, 3> default_ids = {
        globals::default_diffuse_texture().id(),
        globals::default_specular_texture().id(),
        globals::default_normal_texture().id(),
    };

    const auto apply_materials = [&](Entity e, RawProgram<> sp, Location shininess_loc)
    {
        auto tex_ids   = default_ids;
        auto specpower = 128.f;
        override_material({ registry, e }, tex_ids, specpower);

        sp.uniform(shininess_loc, specpower);
        glapi::bind_texture_units(tex_ids);
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

            _skinning_mats.restage(pose.skinning_mats);
            _skinning_mats.bind_to_ssbo_index(0);

            // TODO: Could batch if had SkinStorage.
            draw_one_from_storage(*mesh_storage, bva, bsp, bfb, skinned_mesh.lods.cur());
        }
    };

    // Not Alpha-Tested. Opaque.
    // Can be backface culled.
    if (backface_culling) glapi::enable(Capability::FaceCulling);
    else                  glapi::disable(Capability::FaceCulling);

    draw_from_view(_sp_opaque.get(), view_opaque);

    // Alpha-Tested.
    // No backface culling even if requested.
    glapi::disable(Capability::FaceCulling);
    draw_from_view(_sp_atested.get(), view_atested);
}


} // namespace josh
