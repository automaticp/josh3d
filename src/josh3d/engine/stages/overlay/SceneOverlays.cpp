#include "SceneOverlays.hpp"
#include "BoundingSphere.hpp"
#include "GLProgram.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "TerrainChunk.hpp"
#include "UniformTraits.hpp"
#include "SceneGraph.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/fwd.hpp>


namespace josh::stages::overlay {


void SceneOverlays::operator()(RenderEngineOverlayInterface& engine) {
    draw_selected_highlight(engine);
    draw_bounding_volumes  (engine);
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
            RawProgram<> sp = sp_highlight_stencil_prep_;
            BindGuard bound_program = sp.use();

            const Location model_loc = sp.get_uniform_location("model");


            for (GLint object_mask{ 255 };
                const auto entity : registry.view<Selected, MTransform>())
            {

                // Draws either a singular Mesh, or all Meshes in a subtree
                // as a *single object*. Multiple meshes of a selected subtree
                // will share the same outline without overlap.
                auto draw_func = [&](entt::entity entity) {
                    const entt::const_handle handle{ registry, entity };

                    traverse_subtree_preorder(handle, [&](entt::const_handle node) {
                        if (auto mtf = node.try_get<MTransform>()) {
                            if (auto mesh = node.try_get<Mesh>()) {

                                sp.uniform(model_loc, mtf->model());
                                mesh->draw(bound_program, bound_fbo);

                            } else if (auto terrain_chunk = node.try_get<TerrainChunk>()) {

                                sp.uniform(model_loc, mtf->model());
                                terrain_chunk->mesh.draw(bound_program, bound_fbo);

                            } else if (auto plight = node.try_get<PointLight>()) {

                                // TODO: This probably won't work that well...
                                sp.uniform(model_loc, mtf->model());
                                engine.primitives().sphere_mesh().draw(bound_program, bound_fbo);

                            }
                        }
                    });
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

                glapi::set_polygon_rasterization_mode(PolygonRaserization::Line);
                glapi::set_line_width(2.f * params.outline_width); // Times 2 cause half is cut by inner fill.
                glapi::enable(Capability::AntialiasedLines); // I don't think this works at all.


                draw_func(entity);


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

                glapi::set_polygon_rasterization_mode(PolygonRaserization::Fill);

                draw_func(entity);


                if (object_mask > 1) { --object_mask; }
            }


        }

        glapi::set_color_mask(true, true, true, true);




        // Now just draw a quad twice:
        // Once for the outlines (any value >= 2), then again for the fill (value of 0).

        glapi::enable(Capability::Blending);
        glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        {
            BindGuard bound_program = sp_highlight_->use();


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
            sp_highlight_->uniform("color", params.outline_color);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);


            // Inner Fill.
            glapi::set_stencil_test_condition(Mask{ 0xFF }, 0, CompareOp::Equal);
            glapi::set_stencil_test_operations(
                StencilOp::Keep, // sfail
                StencilOp::Keep, // spass->dfail
                StencilOp::Keep  // spass->dpass
            );
            sp_highlight_->uniform("color", params.inner_fill_color);
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
    glapi::set_polygon_rasterization_mode(PolygonRaserization::Line);
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
    glapi::set_polygon_rasterization_mode(PolygonRaserization::Fill);

}






} // namespace josh::stages::overlay
