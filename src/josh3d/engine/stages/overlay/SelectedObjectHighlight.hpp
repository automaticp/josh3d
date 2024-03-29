#pragma once
#include "DefaultResources.hpp"
#include "GLObjects.hpp"
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
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
private:
    UniqueShaderProgram sp_stencil_prep_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/ovl_selected_stencil_prep.frag"))
            .get()
    };

    UniqueShaderProgram sp_highlight_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_selected_highlight.frag"))
            .get()
    };

public:
    bool show_overlay{ true };

    glm::vec4 outline_color   { 1.0f, 0.612f, 0.0f, 0.8f };
    float     outline_width   { 4.f };
    glm::vec4 inner_fill_color{ 1.0f, 0.612f, 0.0f, 0.2f };

    void operator()(RenderEngineOverlayInterface& engine);

};




inline void SelectedObjectHighlight::operator()(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();

    if (!show_overlay) { return; }
    if (registry.view<tags::Selected>().empty()) { return; }

    using namespace gl;

    engine.draw([&, this] {

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);


        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);


        glStencilMask(0xFF);
        glClearStencil(1); // Sentinel for background, has no outline or fill.
        glClear(GL_STENCIL_BUFFER_BIT);


        // First prepare the stencil buffer.

        sp_stencil_prep_.use()
            .uniform("view",               engine.camera().view_mat())
            .uniform("projection",         engine.camera().projection_mat())
            .and_then([&](ActiveShaderProgram<GLMutable>& ashp) {


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

            for (
                GLint object_mask{ 255 };
                auto [e, world_mtf] : registry.view<tags::Selected, components::MTransform>().each()
            ) {

                // Draws either a singular Mesh, or all Meshes in a Model
                // as a *single object*. Multiple meshes of a selected Model
                // will share the same outline without overlap.
                auto draw_func = [&](entt::entity e) {
                    if (auto mesh = registry.try_get<components::Mesh>(e)) {

                        ashp.uniform("model", world_mtf.model());
                        mesh->draw();

                    } else if (auto model = registry.try_get<components::Model>(e)) {

                        for (const entt::entity& mesh_ent : model->meshes()) {

                            const auto& mesh_world_mtf = registry.get<components::MTransform>(mesh_ent);
                            ashp.uniform("model", mesh_world_mtf.model());
                            registry.get<components::Mesh>(mesh_ent).draw();

                        }

                    } else if (auto terrain_chunk = registry.try_get<components::TerrainChunk>(e)) {

                        ashp.uniform("model", world_mtf.model());
                        terrain_chunk->mesh.draw();

                    }
                };

                // Draw outline as lines, replacing everything except
                // outlines of the previously drawn objects.

                /*
                    if (object_mask >= stencil) {
                        stencil = object_mask;
                    }
                */
                glStencilFunc(GL_GEQUAL, object_mask, 0xFF);
                glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

                // We use this instead of mesh.draw(GL_LINES)
                // because the meshes have the vertex and index information
                // for a GL_TRIANGLES draws, not GL_LINES.
                // Trying to mesh.draw(GL_LINES) results in missing edges.
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(2.f * outline_width); // Times 2 cause half is cut by inner fill.
                glEnable(GL_LINE_SMOOTH);

                draw_func(e);


                // Solid-Fill the insides of the object by drawing it again,
                // but also overwrite the inner half of the outline that we
                // had drawn for this object, and only for this object.

                /*
                    if (object_mask >= stencil) {
                        stencil = 0;
                    }
                */
                glStencilFunc(GL_GEQUAL, object_mask, 0xFF);
                glStencilOp(GL_KEEP, GL_ZERO, GL_ZERO);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                draw_func(e);


                if (object_mask > 1) { --object_mask; }
            }

        });


        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


        // Now just draw a quad twice:
        // Once for the outlines (any value >= 2), then again for the fill (value of 0).

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        sp_highlight_.use()
            .uniform("color", outline_color)
            .and_then([&] {
                /*
                    if (2 <= stencil) {
                        // Draw
                    }
                */
                glStencilFunc(GL_LEQUAL, 2, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                globals::quad_primitive_mesh().draw();
            })
            .uniform("color", inner_fill_color)
            .and_then([&] {
                glStencilFunc(GL_EQUAL, 0, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                globals::quad_primitive_mesh().draw();
            });


        glDisable(GL_BLEND);


        glDisable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);

    });

}




} // namespace josh::stages::overlay
