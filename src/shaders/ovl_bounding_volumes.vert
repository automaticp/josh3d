#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;

uniform mat4 model;


void main() {
    gl_Position = camera.projview * model * vec4(in_pos, 1.0);
}
