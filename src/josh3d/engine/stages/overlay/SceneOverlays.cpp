#include "SceneOverlays.hpp"
#include "AABB.hpp"
#include "BoundingSphere.hpp"
#include "Components.hpp"
#include "ECS.hpp"
#include "GLAPICore.hpp"
#include "GLProgram.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "RenderEngine.hpp"
#include "SkinnedMesh.hpp"
#include "TerrainChunk.hpp"
#include "UniformTraits.hpp"
#include "SceneGraph.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/zip_with.hpp>
#include <ranges>


namespace josh::stages::overlay {


void SceneOverlays::operator()(RenderEngineOverlayInterface& engine) {
    draw_selected_highlight(engine);
    draw_bounding_volumes  (engine);
    draw_scene_graph_lines (engine);
    draw_skeleton          (engine);
}



void SceneOverlays::draw_selected_highlight(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();
    const auto& params   = selected_highlight_params;

    if (!params.show_overlay) { return; }
    if (registry.view<Selected>().empty()) { return; }

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();


    engine.draw([&, this](auto bound_fbo) {

        glapi::disable(Capability::DepthTesting);
        glapi::enable(Capability::StencilTesting);

        glapi::set_color_mask(false, false, false, false);

        glapi::set_stencil_mask(0xFF);
        glapi::clear_stencil_buffer(bound_fbo, 1); // Sentinel for background, has no outline or fill.


        // First prepare the stencil buffer.

        // Object mask is used to uniquely identify object outlines.
        // This supports up to 254 objects for an 8-bit stencil buffer.
        // The values in the buffer will be from 255 to 2, everything below
        // gets clamped to 2, so outlines will just look wrong, no UB.
        // (Why are you selecting so many objects anyway?)
        //
        // Value 0 is reserved for the inner fill of the objects and
        // can stay the same for all of them. We can set it directly
        // with the GL_ZERO stencil OP, and it's the only value we can
        // set directly, and not from reference :(
        //
        // Value 1 is background and left unchanged from the initial clear.
        //
        // We go "down" from 255 to 2 because that allows us to GL_ZERO
        // the values in the solid fill-phase for values <= current object mask,
        // and that will include 0 and 1 too. Also, importantly, this excludes
        // previously drawn outlines, so they are not overwritten.
        {
            const RawProgram<> sp_static  = sp_highlight_stencil_prep_;
            const RawProgram<> sp_skinned = sp_highlight_stencil_prep_skinned_;

            const Location model_static_loc  = sp_static .get_uniform_location("model");
            const Location model_skinned_loc = sp_skinned.get_uniform_location("model");

            for (GLint object_mask{ 255 };
                const Entity entity : registry.view<Selected, MTransform>())
            {
                const CHandle handle{ registry, entity };

                // First, we gather all entities to be drawn into lists.
                // This will let us batch better when drawing.
                thread_local std::vector<Entity> drawlist_static_meshes;  drawlist_static_meshes.clear();
                thread_local std::vector<Entity> drawlist_skinned_meshes; drawlist_skinned_meshes.clear();
                thread_local std::vector<Entity> drawlist_terrain_chunks; drawlist_terrain_chunks.clear();
                thread_local std::vector<Entity> drawlist_plights;        drawlist_plights.clear();

                traverse_subtree_preorder(handle, [&](CHandle node) {
                    if (has_component<MTransform>(node)) {
                        if (has_component<Mesh>        (node)) { drawlist_static_meshes .emplace_back(node); }
                        if (has_component<SkinnedMesh> (node)) { drawlist_skinned_meshes.emplace_back(node); }
                        if (has_component<TerrainChunk>(node)) { drawlist_terrain_chunks.emplace_back(node); }
                        if (has_component<PointLight>  (node)) { drawlist_plights       .emplace_back(node); }
                    }
                });

                // Draws either a singular entity, or all entities in a subtree
                // as a *single object*. Multiple entities of a selected subtree
                // will share the same outline without overlap.
                // This is because their stencil value will be the same.
                auto draw_subtree = [&]{
                    // NOTE: We still have to switch programs between batches.
                    // Could switch stencil values instead, but whatever for now.

                    // TODO: This could be easily done with multidraw,
                    // assuming the MeshID exists for each mesh.
                    if (drawlist_static_meshes.size()  ||
                        drawlist_terrain_chunks.size() ||
                        drawlist_plights.size())
                    {
                        const RawProgram<> sp        = sp_static;
                        const Location     model_loc = model_static_loc;

                        BindGuard bound_program = sp.use();

                        for (const Entity entity : drawlist_static_meshes) {
                            const auto [mesh, mtf] = registry.get<Mesh, MTransform>(entity);
                            sp.uniform(model_loc, mtf.model());
                            mesh.draw(bound_program, bound_fbo);
                        }

                        for (const Entity entity : drawlist_terrain_chunks) {
                            const auto [terrain, mtf] = registry.get<TerrainChunk, MTransform>(entity);
                            sp.uniform(model_loc, mtf.model());
                            terrain.mesh.draw(bound_program, bound_fbo);
                        }

                        for (const Entity entity : drawlist_plights) {
                            const auto [plight, mtf] = registry.get<PointLight, MTransform>(entity);
                            // TODO: This probably won't work that well...
                            sp.uniform(model_loc, mtf.model());
                            engine.primitives().sphere_mesh().draw(bound_program, bound_fbo);
                        }
                    }

                    if (drawlist_skinned_meshes.size()) {
                        const RawProgram<> sp        = sp_skinned;
                        const Location     model_loc = model_skinned_loc;

                        BindGuard bound_program = sp.use();

                        const auto& storage = *engine.meshes().storage_for<VertexSkinned>();
                        BindGuard bound_vao = storage.vertex_array().bind();

                        for (const Entity entity : drawlist_skinned_meshes) {
                            const auto [skinned_mesh, mtf] = registry.get<SkinnedMesh, MTransform>(entity);

                            // TODO: TBH the skinning palette should probably be in some
                            // global buffer similar to MeshStorage but for skin matrices.
                            // Then we can multidraw all the skinned meshes too, heheee...
                            skinning_mats_.restage(skinned_mesh.pose.skinning_mats);
                            skinning_mats_.bind_to_ssbo_index(0);

                            sp.uniform(model_loc, mtf.model());
                            auto [byte_offset, count, basevert] = storage.query_one(skinned_mesh.mesh_id);
                            glapi::draw_elements_basevertex(
                                bound_vao,
                                bound_program,
                                bound_fbo,
                                storage.primitive_type(),
                                storage.element_type(),
                                byte_offset,
                                count,
                                basevert
                            );
                        }

                    }

                };


                // Draw outline as lines, replacing everything except
                // outlines of the previously drawn objects.

                /*
                    if (object_mask >= stencil) {
                        stencil = object_mask;
                    }
                */
                glapi::set_stencil_test_condition(Mask{ 0xFF }, object_mask, CompareOp::GEqual);
                glapi::set_stencil_test_operations(
                    StencilOp::Keep,           // sfail
                    StencilOp::ReplaceWithRef, // spass->dfail
                    StencilOp::ReplaceWithRef  // spass->dpass
                );

                // We use this instead of mesh.draw(GL_LINES)
                // because the meshes have the vertex and index information
                // for a GL_TRIANGLES draws, not GL_LINES.
                // Trying to mesh.draw(GL_LINES) results in missing edges.

                glapi::set_polygon_rasterization_mode(PolygonRasterization::Line);
                glapi::set_line_width(2.f * params.outline_width); // Times 2 cause half is cut by inner fill.
                glapi::enable(Capability::AntialiasedLines); // I don't think this works at all.


                draw_subtree();


                // Solid-Fill the insides of the object by drawing it again,
                // but also overwrite the inner half of the outline that we
                // had drawn for this object, and only for this object.
                //
                // Zero stands for solid-fill.

                /*
                    if (object_mask >= stencil) {
                        stencil = 0;
                    }
                */

                glapi::set_stencil_test_condition(Mask{ 0xFF }, object_mask, CompareOp::GEqual);
                glapi::set_stencil_test_operations(
                    StencilOp::Keep,    // sfail
                    StencilOp::SetZero, // spass->dfail
                    StencilOp::SetZero  // spass->dpass
                );

                glapi::set_polygon_rasterization_mode(PolygonRasterization::Fill);

                draw_subtree();


                if (object_mask > 1) { --object_mask; }
            }


        }

        glapi::set_color_mask(true, true, true, true);




        // Now just draw a quad twice:
        // Once for the outlines (any value >= 2), then again for the fill (value of 0).

        glapi::enable(Capability::Blending);
        glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        {
            const RawProgram<> sp = sp_highlight_;
            BindGuard bound_program = sp.use();


            // Outline.
            /*
                if (2 <= stencil) {
                    // Draw
                }
            */
            glapi::set_stencil_test_condition(Mask{ 0xFF }, 2, CompareOp::LEqual);
            glapi::set_stencil_test_operations(
                StencilOp::Keep, // sfail
                StencilOp::Keep, // spass->dfail
                StencilOp::Keep  // spass->dpass
            );
            sp.uniform("color", params.outline_color);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);


            // Inner Fill.
            glapi::set_stencil_test_condition(Mask{ 0xFF }, 0, CompareOp::Equal);
            glapi::set_stencil_test_operations(
                StencilOp::Keep, // sfail
                StencilOp::Keep, // spass->dfail
                StencilOp::Keep  // spass->dpass
            );
            sp.uniform("color", params.inner_fill_color);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);

        }



        glapi::disable(Capability::Blending);

        glapi::disable(Capability::StencilTesting);
        glapi::enable(Capability::DepthTesting);

    });

}




void SceneOverlays::draw_bounding_volumes(
    RenderEngineOverlayInterface& engine)
{
    using glm::vec3, glm::mat4;
    const auto& registry = engine.registry();
    const auto& params   = bounding_volumes_params;

    if (!params.show_volumes) { return; }
    if (params.selected_only && registry.view<Selected>().empty()) { return; }


    const RawProgram<> sp = sp_bounding_volumes_;

    glapi::enable(Capability::DepthTesting);
    glapi::set_polygon_rasterization_mode(PolygonRasterization::Line);
    glapi::set_line_width(params.line_width);

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();
    sp.uniform("color", params.line_color);


    BindGuard bound_program = sp.use();

    engine.draw([&](auto bound_fbo) {

        const Location model_loc = sp.get_uniform_location("model");

        auto draw_aabb = [&] (
            const entt::entity&,
            const AABB&          aabb)
        {
            const mat4 world_mat = glm::translate(aabb.midpoint()) * glm::scale(aabb.extents() / 2);

            sp.uniform(model_loc, world_mat);

            engine.primitives().box_mesh().draw(bound_program, bound_fbo);
        };

        auto draw_sphere = [&] (
            const entt::entity&,
            const BoundingSphere& sphere)
        {
            const mat4 world_mat = glm::translate(sphere.position) * glm::scale(vec3(sphere.radius));

            sp.uniform(model_loc, world_mat);

            engine.primitives().sphere_mesh().draw(bound_program, bound_fbo);
        };


        if (params.selected_only) {
            registry.view<Selected, AABB>()          .each(draw_aabb);
            registry.view<Selected, BoundingSphere>().each(draw_sphere);
        } else {
            registry.view<AABB>()          .each(draw_aabb);
            registry.view<BoundingSphere>().each(draw_sphere);
        }
    });

    glapi::disable(Capability::DepthTesting);
    glapi::set_polygon_rasterization_mode(PolygonRasterization::Fill);

}




void SceneOverlays::draw_scene_graph_lines(
    RenderEngineOverlayInterface& engine)
{
    using glm::vec3, glm::mat4;
    const auto& registry = engine.registry();
    const auto& params   = scene_graph_lines_params;

    if (!params.show_lines) { return; }
    if (params.selected_only && registry.view<Selected>().empty()) { return; }

    // Update the buffer.
    lines_buf_.clear();

    auto get_line_point = [&](entt::const_handle node) {
        if (params.use_aabb_midpoints && has_component<AABB>(node)) {
            return node.get<AABB>().midpoint();
        } else {
            return node.get<MTransform>().decompose_position();
        }
    };

    auto get_line = [&](entt::const_handle child) {
        const vec3 start = get_line_point(get_parent_handle(child));
        const vec3 end   = get_line_point(child);
        return LineGPU{ start, end };
    };

    auto connectable = [](entt::const_handle node) {
        if (const auto parent = get_parent_handle(node)) {
            return
                has_component<MTransform>(node) &&
                has_component<MTransform>(parent);
        }
        return false;
    };

    // NOTE: This is somewhat braindead and should maybe be better.
    // Especially, since it iterates the same connections if multiple
    // overlapping subtrees of the same tree are selected.
    //
    // Something like the "highest common ancestor" from gizmos might be used
    // but I don't want to touch node containers without node_allocator.
    for (const auto entity : registry.view<Selected, MTransform>()) {
        const entt::const_handle handle{ registry, entity };

        traverse_subtree_preorder(handle, [&](entt::const_handle node) {
            lines_buf_.stage(
                view_child_handles(node)        |
                std::views::filter(connectable) |
                std::views::transform(get_line)
            );
        });

    }

    const RawProgram<> sp = sp_scene_graph_lines_;

    BindGuard bound_cam   = engine.bind_camera_ubo();
    BindGuard bound_lines = lines_buf_.bind_to_ssbo_index(0);

    sp.uniform("color",     params.line_color);
    sp.uniform("dash_size", params.dash_size);

    BindGuard bound_program = sp.use();
    BindGuard bound_vao     = empty_vao_->bind();

    glapi::enable(Capability::DepthTesting);
    glapi::enable(Capability::Blending);
    glapi::set_line_width(params.line_width);

    engine.draw([&](auto bound_fbo) {

        glapi::draw_arrays(
            bound_vao,
            bound_program,
            bound_fbo,
            Primitive::Lines,
            0,
            2 * lines_buf_.num_staged()
        );

    });

    glapi::disable(Capability::DepthTesting);
    glapi::disable(Capability::Blending);

}




void SceneOverlays::draw_skeleton(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();
    const auto& params   = skeleton_params;

    if (!params.show_skeleton) { return; }
    if (params.selected_only && registry.view<Selected>().empty()) { return; }

    /*
    Joints are drawn as simple spheres at joint positions
    Bones are rectangles or cylinders connecting the joints.

    The joint transforms are given by the M2J matrices that are stored
    as part of the pose for convenience in cases like these.

    The bone transforms are another story, however...

    */

    BindGuard bound_cam = engine.bind_camera_ubo();

    // TODO: Can't properly depth-test, since we want to draw bone
    // and joint primitives both on-top *and* with depth testing.
    // Will settle for a constant color for now, but might be possible
    // to reexpress the geometry analytically in the fragment shader.
    glapi::disable(Capability::DepthTesting);

    engine.draw([&](auto bound_fbo) {

        // Draw bones.
        {
            BindGuard bound_vao = empty_vao_->bind();

            // NOTE: Reusing the shader for scene graph lines.
            const RawProgram<> sp = sp_scene_graph_lines_.get();
            BindGuard bound_program = sp.use();

            sp.uniform("color",     params.bone_color);
            sp.uniform("dash_size", params.bone_dash_size);

            auto draw_bones = [&](
                const Entity&,
                const SkinnedMesh& skinned_mesh,
                const MTransform&  mtf)
            {
                using std::views::iota, std::views::transform;

                const auto& pose = skinned_mesh.pose;

                const auto get_line = [&](size_t j) -> LineGPU {
                    const size_t parent_id = pose.skeleton->joints[j].parent_id;
                    const vec3 end   = mtf.model() * pose.M2Js[j][3]; // Last column for position.
                    const vec3 start = mtf.model() * pose.M2Js[parent_id][3];
                    return { start, end };
                };

                bone_lines_buf_.restage(
                    iota(size_t{ 1 }, pose.M2Js.size()) | // Skip root.
                    transform(get_line)
                );

                bone_lines_buf_.bind_to_ssbo_index(0);

                glapi::draw_arrays(
                    bound_vao,
                    bound_program,
                    bound_fbo,
                    Primitive::Lines,
                    0,
                    2 * bone_lines_buf_.num_staged()
                );
            };

            glapi::enable(Capability::Blending);
            glapi::set_line_width(params.bone_width);

            if (params.selected_only) {
                registry.view<Selected, SkinnedMesh, MTransform>().each(draw_bones);
            } else {
                registry.view<SkinnedMesh, MTransform>().each(draw_bones);
            }

            glapi::disable(Capability::Blending);
        }


        // Draw joints.
        {
            const Mesh& sphere = engine.primitives().sphere_mesh();
            BindGuard bound_vao = sphere.vertex_array().bind();

            const RawProgram<> sp = sp_skeleton_.get();
            BindGuard bound_program = sp.use();

            sp.uniform("color", params.joint_color);

            auto draw_joints = [&](
                const Entity&,
                const SkinnedMesh& skinned_mesh,
                const MTransform&  mtf)
            {
                sp.uniform("model", mtf.model());

                using std::views::transform;
                const auto rescale = [&](const mat4& m) { return glm::scale(m, vec3(params.joint_scale)); };

                joint_tfs_.restage(
                    skinned_mesh.pose.M2Js |
                    transform(rescale)
                );

                BindGuard bound_tfs = joint_tfs_.bind_to_ssbo_index(0);

                glapi::draw_elements_instanced(
                    bound_vao,
                    bound_program,
                    bound_fbo,
                    joint_tfs_.num_staged(),
                    sphere.primitive_type(),
                    sphere.element_type(),
                    sphere.element_offset_bytes(),
                    sphere.num_elements()
                );
            };

            if (params.selected_only) {
                registry.view<Selected, SkinnedMesh, MTransform>().each(draw_joints);
            } else {
                registry.view<SkinnedMesh, MTransform>().each(draw_joints);
            }
        }

    });

    glapi::disable(Capability::DepthTesting);

}


} // namespace josh::stages::overlay
