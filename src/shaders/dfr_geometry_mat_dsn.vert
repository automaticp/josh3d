#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;
layout (location = 3) in vec3 in_tangent;

uniform mat4 model;
uniform mat3 normal_model;

out vec2 tex_coords;
out mat3 TBN;
out vec3 frag_pos;


void main() {
    tex_coords = in_tex_coords;

    frag_pos = vec3(model * vec4(in_pos, 1.0));

    gl_Position = camera.projview * vec4(frag_pos, 1.0);

    // Gram-Schmidt renormalization.

    vec3 T = normalize(normal_model * in_tangent);
    vec3 N = normalize(normal_model * in_normal);

    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);
}
