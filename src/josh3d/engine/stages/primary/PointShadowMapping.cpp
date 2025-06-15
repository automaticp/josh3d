#include "PointShadowMapping.hpp"
#include "GLAPIBinding.hpp"
#include "GLProgram.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "StaticMesh.hpp"
#include "UniformTraits.hpp"
#include "BoundingSphere.hpp"
#include "VertexStatic.hpp"
#include "tags/ShadowCasting.hpp"
#include "tags/AlphaTested.hpp"
#include "Materials.hpp"
#include "Transform.hpp"
#include "LightCasters.hpp"
#include "tags/Visible.hpp"
#include "ECS.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>


namespace josh {


PointShadowMapping::PointShadowMapping(i32 side_resolution)
{
    // This will just set the maps._side_resolution.
    // No storage will be allocated because of 0.
    point_shadows.maps._resize(side_resolution, 0);
}

void PointShadowMapping::operator()(
    RenderEnginePrimaryInterface& engine)
{
    prepare_point_shadows(engine.registry());

    map_point_shadows(engine);

    engine.belt().put_ref(point_shadows);
}

void PointShadowMapping::prepare_point_shadows(
    const Registry& registry)
{
    auto plights_with_shadow =
        registry.view<Visible, ShadowCasting, PointLight, MTransform, BoundingSphere>();

    // This technically makes a redundant iteration over the view
    // because getting the size from a view is an O(n) operation.
    //
    // The reality, however is that the number of point lights
    // with shadows in your scene, is not likely to be more than
    // ~10, and even then you're probably already pushing it too far.
    //
    // You could do a stupid thing and use a size_hint() of a view,
    // which is O(1), but then you'd be severly overestimating
    // the number of actual point lights in the scene, and with that,
    // the number of cubemaps needed to be allocated. Given that
    // a single depth cubemap is actually really big in memory,
    // asking for more than you need is a truly bad idea.
    //
    // UPD: We also use this opportunity to populate the entities
    // and views array in the output structure. It's even less useless now!

    i32 num_cubes = 0;
    point_shadows.entities.clear();
    point_shadows.views.clear();
    for (const auto [e, plight, mtf, sphere] : plights_with_shadow.each())
    {
        // TODO: z_near is hardcoded, but could be scaled from point light radius instead?
        const float z_near = 0.05f;
        const float z_far  = sphere.radius;

        const mat4 proj =
            glm::perspective(glm::half_pi<float>(), 1.0f, z_near, z_far);

        const vec3 pos = mtf.decompose_position();

        point_shadows.views.push_back({
            .z_near    = z_near,
            .z_far     = z_far,
            .proj_mat  = proj,
            .view_mats = {
                glm::lookAt(pos, pos + X, -Y),
                glm::lookAt(pos, pos - X, -Y),
                glm::lookAt(pos, pos + Y,  Z),
                glm::lookAt(pos, pos - Y, -Z),
                glm::lookAt(pos, pos + Z, -Y),
                glm::lookAt(pos, pos - Z, -Y),
            }
        });
        point_shadows.entities.push_back(e);
        ++num_cubes;
    }

    point_shadows.maps._resize(side_resolution(), num_cubes);
}

void PointShadowMapping::map_point_shadows(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    const auto& mesh_registry = engine.meshes();
    auto& maps = point_shadows.maps;

    if (maps.num_cubes() == 0)
        return;

    glapi::set_viewport({ {}, maps.resolution() });

    fbo_->attach_texture_to_depth_buffer(maps.cubemaps());
    const BindGuard bfb = fbo_->bind_draw();

    glapi::clear_depth_buffer(bfb, 1.f);

    const auto set_per_light_uniforms = [&](RawProgram<> sp, uindex cubemap_id)
    {
        const auto& view = point_shadows.views[cubemap_id];

        // HMM: This could certainly be sent over UBO, but we are *far*
        // from this being the primary bottleneck.
        const Location views_loc = sp.get_uniform_location("views");
        sp.set_uniform_mat4v(views_loc, 6, false, value_ptr(view.view_mats[0]));

        sp.uniform("projection", view.proj_mat);
        sp.uniform("cubemap_id", i32(cubemap_id));
        sp.uniform("z_far",      view.z_far);
    };

    // Alpha-tested.
    {
        const RawProgram<> sp  = sp_with_alpha_;
        const BindGuard    bsp = sp.use();

        for (const uindex cubemap_idx : irange(num_cubes()))
        {
            set_per_light_uniforms(sp, cubemap_idx);
            draw_all_world_geometry_with_alpha_test(bsp, bfb, mesh_registry, registry);
        }
    }

    // Opaque.
    {
        const RawProgram<> sp  = sp_no_alpha_;
        const BindGuard    bsp = sp.use();

        for (const uindex cubemap_idx : irange(num_cubes()))
        {
            set_per_light_uniforms(sp, cubemap_idx);
            draw_all_world_geometry_no_alpha_test(bsp, bfb, mesh_registry, registry);
        }
    }
}

void PointShadowMapping::draw_all_world_geometry_no_alpha_test(
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry)
{
    // Assumes that projection and view are already set.
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    if (not storage) return;

    const BindGuard bva = storage->vertex_array().bind();

    const RawProgram<> sp = sp_no_alpha_;

    // TODO: Could easily multidraw this.
    const auto draw_from_view = [&](auto view)
    {
        const Location model_loc = sp.get_uniform_location("model");
        for (const auto [entity, world_mtf, mesh] : view.each())
        {
            sp.uniform(model_loc, world_mtf.model());
            draw_one_from_storage(*storage, bva, bsp, bfb, mesh.lods.cur());
        }
    };

    // You could have no AT requested, or you could have an AT flag,
    // but no diffuse material to sample from. Both ignore Alpha-Testing.

    // FIXME: I don't like the above idea that much anymore. Do *one* thing.
    // If it's tagged AlphaTested, then it *will* be alpha-tested.
    // If you don't like that then fix whatever code incorrectly flags stuff as AT.

    // TODO: Opaque should be a tag assigned to all entities that do *not*
    // have AlphaTested or Transparent. Otherwise we are doing negative filtering.

    draw_from_view(registry.view<MTransform, StaticMesh>(entt::exclude<AlphaTested>));
    draw_from_view(registry.view<MTransform, StaticMesh, AlphaTested>(entt::exclude<MaterialDiffuse>));
}

void PointShadowMapping::draw_all_world_geometry_with_alpha_test(
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry)
{

    // Assumes that projection and view are already set.
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    if (not storage) return;

    const BindGuard bva = storage->vertex_array().bind();

    // TODO: Could be a simple place to try batch-draws.
    const RawProgram<> sp = sp_with_alpha_;
    sp.uniform("material.diffuse", 0);

    auto meshes_with_alpha_view =
        registry.view<AlphaTested, StaticMesh, MaterialDiffuse, MTransform>();

    const Location model_loc = sp.get_uniform_location("model");
    for (auto [entity, mesh, diffuse, world_mtf]
        : meshes_with_alpha_view.each())
    {
        diffuse.texture->bind_to_texture_unit(0);
        sp.uniform(model_loc, world_mtf.model());
        draw_one_from_storage(*storage, bva, bsp, bfb, mesh.lods.cur());
    }
}

void PointShadowMapping::resize_maps(i32 side_resolution)
{
    point_shadows.maps._resize(side_resolution, point_shadows.maps.num_cubes());
}


} // namespace josh
