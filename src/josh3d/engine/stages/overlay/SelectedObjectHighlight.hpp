#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "SceneGraph.hpp"
#include "ShaderBuilder.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include "Mesh.hpp"
#include "VPath.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <entt/entt.hpp>




namespace josh::stages::overlay {


class SelectedObjectHighlight {
public:
    bool show_overlay{ true };

    float     outline_width   { 3.f };
    glm::vec4 outline_color   { 0.0f, 0.0f,   0.0f, 0.784f };
    glm::vec4 inner_fill_color{ 1.0f, 0.612f, 0.0f, 0.392f };

    void operator()(RenderEngineOverlayInterface& engine);



private:
    UniqueProgram sp_stencil_prep_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/ovl_selected_stencil_prep.frag"))
            .get()
    };

    UniqueProgram sp_highlight_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_selected_highlight.frag"))
            .get()
    };

};




inline void SelectedObjectHighlight::operator()(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();

    if (!show_overlay) { return; }
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
            RawProgram<> sp = sp_stencil_prep_;
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
                glapi::set_line_width(2.f * outline_width); // Times 2 cause half is cut by inner fill.
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
            sp_highlight_->uniform("color", outline_color);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);


            // Inner Fill.
            glapi::set_stencil_test_condition(Mask{ 0xFF }, 0, CompareOp::Equal);
            glapi::set_stencil_test_operations(
                StencilOp::Keep, // sfail
                StencilOp::Keep, // spass->dfail
                StencilOp::Keep  // spass->dpass
            );
            sp_highlight_->uniform("color", inner_fill_color);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);

        }



        glapi::disable(Capability::Blending);

        glapi::disable(Capability::StencilTesting);
        glapi::enable(Capability::DepthTesting);

    });

}




} // namespace josh::stages::overlay
