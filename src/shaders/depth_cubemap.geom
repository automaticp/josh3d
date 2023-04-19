#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 projection;
uniform mat4 views[6];

out vec4 frag_pos;
out vec3 light_pos;

void main() {

    // First face (X+, face_id = 0) preserves the
    // values of light source position, but reverses the
    // coordinates, so we just un-swizzle the light position.
    light_pos = vec3(views[0][3]).zyx;

    for (int face_id = 0; face_id < views.length(); ++face_id) {

        gl_Layer = face_id;
        for (int vertex_id = 0; vertex_id < 3; ++ vertex_id) {

            frag_pos = gl_in[vertex_id].gl_Position;

            gl_Position = projection * views[face_id] * frag_pos;

            EmitVertex();
        }
        EndPrimitive();

    }

}
