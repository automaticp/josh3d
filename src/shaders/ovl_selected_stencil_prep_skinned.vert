#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"
#include "skinning.glsl"

layout (location = 0) in vec3  in_pos;
layout (location = 4) in uvec4 in_joint_ids;
layout (location = 5) in vec4  in_joint_weights;

uniform mat4 model;

void main() {
    const vec4 bind_pos    = vec4(in_pos, 1.0);
    const mat4 skin_mat    = compute_skin_matrix(in_joint_ids, in_joint_weights);
    const vec4 skinned_pos = skin_mat * bind_pos;
    gl_Position = camera.projview * model * skinned_pos;
}
