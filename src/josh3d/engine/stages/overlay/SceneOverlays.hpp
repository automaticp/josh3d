#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "UploadBuffer.hpp"


namespace josh::stages::overlay {


class SceneOverlays {
public:
    struct SelectedHighlightParams {
        bool      show_overlay    { true };
        float     outline_width   { 3.f };
        glm::vec4 outline_color   { 0.0f, 0.0f,   0.0f, 0.784f };
        glm::vec4 inner_fill_color{ 1.0f, 0.612f, 0.0f, 0.392f };
    } selected_highlight_params;

    struct BoundingVolumesParams {
        bool      show_volumes { false };
        bool      selected_only{ true  };
        glm::vec3 line_color   { 0.77f, 0.77f, 0.77f };
        float     line_width   { 3.f };
    } bounding_volumes_params;

    struct SceneGraphLinesParams {
        bool      show_lines   { true };
        bool      selected_only{ true }; // TODO: Support or remove.
        bool      use_aabb_midpoints{ true };
        glm::vec4 line_color   { 0.f, 0.f, 0.f, 0.404f };
        float     line_width   { 3.f };
        float     dash_size    { 0.025f };
    } scene_graph_lines_params;

    void operator()(RenderEngineOverlayInterface& engine);

private:
    void draw_selected_highlight(RenderEngineOverlayInterface& engine);

    UniqueProgram sp_highlight_stencil_prep_{
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


    void draw_bounding_volumes(RenderEngineOverlayInterface& engine);

    UniqueProgram sp_bounding_volumes_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/ovl_bounding_volumes.vert"))
            .load_frag(VPath("src/shaders/ovl_bounding_volumes.frag"))
            .get()
    };


    void draw_scene_graph_lines(RenderEngineOverlayInterface& engine);

    struct LineGPU {
        alignas(std430::align_vec3) glm::vec3 start;
        alignas(std430::align_vec3) glm::vec3 end;
    };

    UploadBuffer<LineGPU> lines_buf_;
    UniqueVertexArray     empty_vao_;

    UniqueProgram sp_scene_graph_lines_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/ovl_dashed_lines.vert"))
            .load_frag(VPath("src/shaders/ovl_dashed_lines.frag"))
            .get()
    };

};


} // namespace josh::stages::overlay
