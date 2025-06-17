#version 430 core

layout (triangles) in;

#ifndef MAX_VERTICES
#define MAX_VERTICES 36 /* 12 * 3 */
#endif

const int max_cascades = MAX_VERTICES / 3;

uniform mat4 projections[max_cascades];
uniform mat4 views[max_cascades];
uniform int  num_cascades;

#ifdef ENABLE_ALPHA_TESTING

in Interface
{
    vec2 uv;
} in_[3];

out Interface
{
    vec2 uv;
} out_;

#endif // ENABLE_ALPHA_TESTING

layout (triangle_strip, max_vertices = MAX_VERTICES) out;


void main()
{
    for (int layer_id = 0; layer_id < num_cascades; ++layer_id)
    {
        for (int vertex_id = 0; vertex_id < 3; ++vertex_id)
        {
#ifdef ENABLE_ALPHA_TESTING
            out_.uv     = in_[vertex_id].uv;
#endif // ENABLE_ALPHA_TESTING
            gl_Position = projections[layer_id] * views[layer_id] * gl_in[vertex_id].gl_Position;
            gl_Layer    = layer_id;

            EmitVertex();
        }
        EndPrimitive();
    }
}
