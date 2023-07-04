#version 430 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_bitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normal_model;

out vec2 tex_coords;
out mat3 TBN;
out vec3 frag_pos;


void main() {
    tex_coords = in_tex_coords;

    frag_pos = vec3(model * vec4(in_pos, 1.0));

    gl_Position = projection * view * vec4(frag_pos, 1.0);

    TBN = mat3(
        vec3(normal_model * in_tangent),
        vec3(normal_model * in_bitangent),
        vec3(normal_model * in_normal)
    );
}
