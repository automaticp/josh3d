#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"


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

    struct SkeletonParams {
        bool  show_skeleton { false };
        bool  selected_only { true  };
        vec3  joint_color   { 1.0f, 1.0f, 0.569f };
        float joint_scale   { 0.1f  };
        vec4  bone_color    { 1.0f, 0.817f, 0.5f, 0.5f };
        float bone_width    { 3.f   };
        float bone_dash_size{ 1.0f  };
    } skeleton_params;

    void operator()(RenderEngineOverlayInterface& engine);

private:
    void draw_selected_highlight(RenderEngineOverlayInterface& engine);

    ShaderToken sp_highlight_stencil_prep_ = shader_pool().get({
        .vert = VPath("src/shaders/basic_mesh.vert"),
        .frag = VPath("src/shaders/ovl_selected_stencil_prep.frag")});

    ShaderToken sp_highlight_stencil_prep_skinned_ = shader_pool().get({
        .vert = VPath("src/shaders/ovl_selected_stencil_prep_skinned.vert"),
        .frag = VPath("src/shaders/ovl_selected_stencil_prep.frag")});

    ShaderToken sp_highlight_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_selected_highlight.frag")});


    void draw_bounding_volumes(RenderEngineOverlayInterface& engine);

    ShaderToken sp_bounding_volumes_ = shader_pool().get({
        .vert = VPath("src/shaders/ovl_bounding_volumes.vert"),
        .frag = VPath("src/shaders/ovl_bounding_volumes.frag")});


    void draw_scene_graph_lines(RenderEngineOverlayInterface& engine);

    struct LineGPU {
        alignas(std430::align_vec3) vec3 start;
        alignas(std430::align_vec3) vec3 end;
    };

    UploadBuffer<LineGPU> lines_buf_;
    UploadBuffer<mat4>    skinning_mats_;
    UniqueVertexArray     empty_vao_;

    ShaderToken sp_scene_graph_lines_ = shader_pool().get({
        .vert = VPath("src/shaders/ovl_dashed_lines.vert"),
        .frag = VPath("src/shaders/ovl_dashed_lines.frag")});


    void draw_skeleton(RenderEngineOverlayInterface& engine);

    ShaderToken sp_skeleton_ = shader_pool().get({
        .vert = VPath("src/shaders/ovl_skeleton_joints.vert"),
        .frag = VPath("src/shaders/ovl_skeleton_joints.frag")});

    UploadBuffer<mat4>    joint_tfs_;
    UploadBuffer<LineGPU> bone_lines_buf_;

};


} // namespace josh::stages::overlay
