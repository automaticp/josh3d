#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "instance_transforms.glsl"


layout (location = 0) in vec3 in_pos;


uniform mat4 projection;
uniform mat4 view;


void main() {
    const mat4 model = instance_transforms[gl_DrawID];
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
