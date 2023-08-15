#version 430 core

#ifndef MAX_VERTICES
    #define MAX_VERTICES 36 /* 12 * 3 */
#endif

layout (triangles) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

const int max_cascades = 3 * MAX_VERTICES;

uniform mat4 projections[max_cascades];
uniform mat4 views[max_cascades];
uniform int num_cascades;


#ifdef ENABLE_ALPHA_TESTING

in Interface {
    vec2 tex_coords;
} in_[3];

#endif // ENABLE_ALPHA_TESTING


out Interface {
    vec4 frag_pos;

#ifdef ENABLE_ALPHA_TESTING

    vec2 tex_coords;

#endif // ENABLE_ALPHA_TESTING
} out_;


void main() {

    for (int layer_id = 0; layer_id < num_cascades; ++layer_id) {
        for (int vertex_id = 0; vertex_id < 3; ++vertex_id) {

#ifdef ENABLE_ALPHA_TESTING
            out_.tex_coords = in_[vertex_id].tex_coords;
#endif // ENABLE_ALPHA_TESTING

            out_.frag_pos = gl_in[vertex_id].gl_Position;

            gl_Position =
                projections[layer_id] * views[layer_id] * out_.frag_pos;

            gl_Layer = layer_id;

            EmitVertex();
        }
        EndPrimitive();
    }

}
