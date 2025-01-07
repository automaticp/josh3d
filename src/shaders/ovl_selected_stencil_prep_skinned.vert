#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3  in_pos;
layout (location = 4) in uvec4 in_joint_ids;
layout (location = 5) in vec4  in_joint_weights;

uniform mat4 model;

layout (std430, binding = 0) restrict readonly
buffer SkinningMatrices {
    mat4 skinning_mats[];
};


void main() {
    const vec4 bind_pos = vec4(in_pos, 1.0);

    const mat4 skin_mat =
        in_joint_weights[0] * skinning_mats[in_joint_ids[0]] +
        in_joint_weights[1] * skinning_mats[in_joint_ids[1]] +
        in_joint_weights[2] * skinning_mats[in_joint_ids[2]] +
        in_joint_weights[3] * skinning_mats[in_joint_ids[3]];

    const vec4 skinned_pos = skin_mat * bind_pos;
    gl_Position = camera.projview * model * skinned_pos;
}
