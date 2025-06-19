#include "DeferredGeometry.hpp"
#include "DrawHelpers.hpp"
#include "GLAPIBinding.hpp"
#include "Ranges.hpp"
#include "UploadBuffer.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "GLAPICore.hpp"
#include "GLProgram.hpp"
#include "MeshStorage.hpp"
#include "StaticMesh.hpp"
#include "UniformTraits.hpp"
#include "Materials.hpp"
#include "AlphaTested.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "DefaultTextures.hpp"
#include "Visible.hpp"


namespace josh {


void DeferredGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    switch (strategy)
    {
        case Strategy::DrawPerMesh: return _draw_single(engine);
        case Strategy::BatchedMDI:     return _draw_batched(engine);
    }
}

void DeferredGeometry::_draw_single(RenderEnginePrimaryInterface &engine)
{
    const auto& registry = engine.registry();
    const auto* mesh_storage = engine.meshes().storage_for<VertexStatic>();
    auto*       gbuffer      = engine.belt().try_get<GBuffer>();

    if (not mesh_storage) return;
    if (not gbuffer)      return;

    const BindGuard bcam = engine.bind_camera_ubo();

    // FIXME: Negative filtering.

    auto view_opaque  = registry.view<Visible, StaticMesh, MTransform>(entt::exclude<AlphaTested>);
    auto view_atested = registry.view<Visible, AlphaTested, StaticMesh, MTransform>();

    const Array<u32, 3> default_units = {
        globals::default_diffuse_texture().id(),
        globals::default_specular_texture().id(),
        globals::default_normal_texture().id(),
    };

    const auto apply_materials = [&](Entity e, RawProgram<> sp, Location shininess_loc)
    {
        auto units     = default_units;
        auto shininess = 128.f;

        if (auto* mat = registry.try_get<MaterialDiffuse>(e))
            units[0] = mat->texture->id();

        if (auto* mat = registry.try_get<MaterialSpecular>(e))
        {
            units[1]  = mat->texture->id();
            shininess = mat->shininess;
        }

        if (auto* mat = registry.try_get<MaterialNormal>(e))
            units[2] = mat->texture->id();

        sp.uniform(shininess_loc, shininess);
        glapi::bind_texture_units(units);
    };

    glapi::set_viewport({ {}, gbuffer->resolution() });

    const BindGuard bfb = gbuffer->bind_draw();
    const BindGuard bva = mesh_storage->vertex_array().bind();

    const auto draw = [&](RawProgram<> sp, auto view)
    {
        const BindGuard bsp = sp.use();

        sp.uniform("material.diffuse",  0);
        sp.uniform("material.specular", 1);
        sp.uniform("material.normal",   2);

        const Location model_loc        = sp.get_uniform_location("model");
        const Location normal_model_loc = sp.get_uniform_location("normal_model");
        const Location object_id_loc    = sp.get_uniform_location("object_id");
        const Location shininess_loc    = sp.get_uniform_location("material.shininess");

        for (auto [entity, mesh, world_mtf] : view.each())
        {
            sp.uniform(model_loc,        world_mtf.model());
            sp.uniform(normal_model_loc, world_mtf.normal_model());
            sp.uniform(object_id_loc,    entt::to_integral(entity));

            apply_materials(entity, sp, shininess_loc);
            draw_one_from_storage(*mesh_storage, bva, bsp, bfb, mesh.lods.cur());
        }
    };

    // Not Alpha-Tested. Opaque.
    // Can be backface culled.

    if (backface_culling) glapi::enable(Capability::FaceCulling);
    else                  glapi::disable(Capability::FaceCulling);

    draw(_sp_single_opaque, view_opaque);

    // Alpha-Tested.
    // No backface culling even if requested.

    glapi::disable(Capability::FaceCulling);
    draw(_sp_single_atested.get(), view_atested);
}

void DeferredGeometry::_draw_batched(RenderEnginePrimaryInterface& engine)
{
    const auto& registry     = engine.registry();
    const auto* mesh_storage = engine.meshes().storage_for<VertexStatic>();
    auto*       gbuffer      = engine.belt().try_get<GBuffer>();

    if (not mesh_storage) return;
    if (not gbuffer)      return;

    const BindGuard bcam = engine.bind_camera_ubo();
    const BindGuard bfb  = gbuffer->bind_draw();
    const BindGuard bva  = mesh_storage->vertex_array().bind();

    glapi::set_viewport({ {}, gbuffer->resolution() });

    // FIXME: Negative filtering.

    auto view_opaque  = registry.view<Visible, StaticMesh, MTransform>(entt::exclude<AlphaTested>);
    auto view_atested = registry.view<Visible, AlphaTested, StaticMesh, MTransform>();

    const usize batch_size = max_batch_size();
    const usize num_units  = _max_texture_units();

    // NOTE: Resizing, not reserving.
    thread_local Vector<u32> tex_units; tex_units.resize(num_units);

    // Need this to set all sampler uniforms in one call.
    const Span<const i32> samplers = build_irange_tls_array(num_units);

    const Array<u32, 3> default_ids = {
        globals::default_diffuse_texture().id(),
        globals::default_specular_texture().id(),
        globals::default_normal_texture().id(),
    };

    const auto draw = [&](RawProgram<> sp, auto view)
    {
        const BindGuard bsp = sp.use();

        const Location samplers_loc = sp.get_uniform_location("samplers");
        sp.set_uniform_intv(samplers_loc, i32(samplers.size()), samplers.data());

        _instance_data.clear();
        uindex draw_id = 0;

        const auto push_instance = [&](Entity e, const MTransform& mtf)
        {
            auto tex_ids   = default_ids;
            auto specpower = 128.f;
            override_material({ registry, e }, tex_ids, specpower);

            _instance_data.stage_one({
                .model        = mtf.model(),
                .normal_model = mtf.normal_model(),
                .object_id    = to_entity(e),
                .specpower    = specpower,
            });

            tex_units[draw_id * 3 + 0] = tex_ids[0];
            tex_units[draw_id * 3 + 1] = tex_ids[1];
            tex_units[draw_id * 3 + 2] = tex_ids[2];
        };

        const auto draw_staged_and_reset = [&]()
        {
            glapi::bind_texture_units(tex_units);
            _instance_data.bind_to_ssbo_index(0);

            // NOTE: Interestingly, we have the Entity already stored in the
            // instance buffer, so we can just look it up from there no problem.
            const auto get_mesh_id = [&](const InstanceDataGPU& data)
            {
                return view.template get<StaticMesh>(Entity(data.object_id)).lods.cur();
            };

            multidraw_indirect_from_storage(*mesh_storage, bva, bsp, bfb,
                _instance_data.view_staged() | transform(get_mesh_id), _mdi_buffer);

            _instance_data.clear();
            draw_id = 0;
        };

        // The draw loop.
        for (auto [e, mesh, mtf] : view.each())
        {
            push_instance(e, mtf);

            // If we overflow the batch, then multidraw and reset.
            if (++draw_id >= batch_size)
                draw_staged_and_reset();
        }
        if (draw_id) draw_staged_and_reset(); // Don't forget the tail.
    };

    // Opaque. Can be backface culled.
    if (backface_culling) glapi::enable(Capability::FaceCulling);
    else                  glapi::disable(Capability::FaceCulling);

    draw(_sp_batched_opaque, view_opaque);

    // Alpha-Tested. No backface culling even if requested.
    glapi::disable(Capability::FaceCulling);
    draw(_sp_batched_atested, view_atested);
}

auto DeferredGeometry::_max_texture_units() const noexcept
    -> u32
{
    return glapi::get_limit(LimitI::MaxFragTextureUnits);
}

auto DeferredGeometry::max_batch_size() const noexcept
    -> u32
{
    return _max_texture_units() / 3; // 3 textures in a material.
}

} // namespace josh
