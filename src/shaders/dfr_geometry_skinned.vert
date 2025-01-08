#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"
#include "skinning.glsl"

out Interface {
    vec2 uv;
    mat3 TBN;
    vec3 frag_pos;
} out_;

layout (location = 0) in vec3  in_pos;
layout (location = 1) in vec2  in_uv;
layout (location = 2) in vec3  in_normal;
layout (location = 3) in vec3  in_tangent;
layout (location = 4) in uvec4 in_joint_ids;
layout (location = 5) in vec4  in_joint_weights;

uniform mat4 model;
uniform mat3 normal_model;


void main() {
    out_.uv = in_uv;

    // Compute weighted average of the resulting vertex positions.
    const vec4 bind_pos        = vec4(in_pos, 1.0);
    const mat4 skin_mat        = compute_skin_matrix(in_joint_ids, in_joint_weights);
    const vec4 skinned_pos     = skin_mat * bind_pos;
    const mat3 normal_skin_mat = mat3(inverse(transpose(skin_mat)));


    out_.frag_pos = vec3(model * skinned_pos);
    gl_Position   = camera.projview * vec4(out_.frag_pos, 1.0);

    // Gram-Schmidt renormalization.

    vec3 T = normalize(normal_model * normal_skin_mat * in_tangent);
    vec3 N = normalize(normal_model * normal_skin_mat * in_normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    out_.TBN = mat3(T, B, N);
}
