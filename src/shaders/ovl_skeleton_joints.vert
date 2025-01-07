#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;

layout (std430, binding = 0) restrict readonly
buffer Transforms {
    mat4 M2Ls[]; // Mesh->Local
};

uniform mat4 model; // World->Mesh


void main() {
    const mat4 W2M = model;
    const mat4 W2L = W2M * M2Ls[gl_InstanceID];
    gl_Position = camera.projview * W2L * vec4(in_pos, 1.0);
}
