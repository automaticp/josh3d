#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18 /* 3 x 6 = 18 */) out;

uniform mat4 projection;
uniform mat4 views[6];
uniform int cubemap_id;

out vec4 frag_pos;
out vec3 light_pos;

void main() {

    // First face (X+, face_id = 0) preserves the values of
    // light source position in the translation column, but reverses the
    // coordinate order, so we just un-swizzle the light position from it.
    vec3 light_pos_value = vec3(views[0][3]).zyx;
    // You can use any other view matrix of the six, but they
    // should be un-swizzled differenty and have the signs possibly flipped.

    for (int face_id = 0; face_id < views.length(); ++face_id) {

        for (int vertex_id = 0; vertex_id < 3; ++vertex_id) {

            frag_pos = gl_in[vertex_id].gl_Position;
            light_pos = light_pos_value;
            gl_Position = projection * views[face_id] * frag_pos;
            // Warning: gl_Layer and gl_ViewportIndex are GS output variables. As such, every time you call
            // EmitVertex, their values will become undefined. Therefore, you must set these variables
            // every time you loop over outputs.
            gl_Layer = 6 * cubemap_id + face_id;
            EmitVertex();
        }
        EndPrimitive();



    }
}
