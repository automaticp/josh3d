#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "components/Model.hpp"
#include "components/TerrainChunk.hpp"
#include "components/Transform.hpp"
#include "tags/Selected.hpp"
#include "components/Mesh.hpp"
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

    glm::vec4 outline_color   { 1.0f, 0.612f, 0.0f, 0.8f };
    float     outline_width   { 4.f };
    glm::vec4 inner_fill_color{ 1.0f, 0.612f, 0.0f, 0.2f };

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
    if (registry.view<tags::Selected>().empty()) { return; }


    engine.draw([&, this](auto bound_fbo) {

        glapi::disable(Capability::DepthTesting);
        glapi::enable(Capability::StencilTesting);

        glapi::set_color_mask(false, false, false, false);

        glapi::set_stencil_mask(0xFF);
        glapi::clear_stencil_buffer(bound_fbo, 1); // Sentinel for background, has no outline or fill.


        // First prepare the stencil buffer.

        sp_stencil_prep_->uniform("view",       engine.camera().view_mat());
        sp_stencil_prep_->uniform("projection", engine.camera().projection_mat());

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

        auto bound_program = sp_stencil_prep_->use();

        for (GLint object_mask{ 255 };
            auto [e, world_mtf] : registry.view<tags::Selected, components::MTransform>().each())
        {

            // Draws either a singular Mesh, or all Meshes in a Model
            // as a *single object*. Multiple meshes of a selected Model
            // will share the same outline without overlap.
            auto draw_func = [&](entt::entity e) {
                if (auto mesh = registry.try_get<components::Mesh>(e)) {

                    sp_stencil_prep_->uniform("model", world_mtf.model());
                    mesh->draw(bound_program, bound_fbo);

                } else if (auto model = registry.try_get<components::Model>(e)) {

                    for (const entt::entity& mesh_ent : model->meshes()) {

                        const auto& mesh_world_mtf = registry.get<components::MTransform>(mesh_ent);
                        sp_stencil_prep_->uniform("model", mesh_world_mtf.model());
                        registry.get<components::Mesh>(mesh_ent).draw(bound_program, bound_fbo);

                    }

                } else if (auto terrain_chunk = registry.try_get<components::TerrainChunk>(e)) {

                    sp_stencil_prep_->uniform("model", world_mtf.model());
                    terrain_chunk->mesh.draw(bound_program, bound_fbo);

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

            glapi::set_polygon_rasterization_mode(PolygonRaserization::Line);
            glapi::set_line_width(2.f * outline_width); // Times 2 cause half is cut by inner fill.
            glapi::enable(Capability::AntialiasedLines); // I don't think this works at all.


            draw_func(e);


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

            draw_func(e);


            if (object_mask > 1) { --object_mask; }
        }

        bound_program.unbind();
        glapi::set_color_mask(true, true, true, true);




        // Now just draw a quad twice:
        // Once for the outlines (any value >= 2), then again for the fill (value of 0).

        glapi::enable(Capability::Blending);
        glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        {
            auto bound_program = sp_highlight_->use();


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


            bound_program.unbind();
        }



        glapi::disable(Capability::Blending);

        glapi::disable(Capability::StencilTesting);
        glapi::enable(Capability::DepthTesting);

    });

}




} // namespace josh::stages::overlay
