#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

out vec2 tex_coords;
out vec3 normal;
out vec3 frag_pos;

layout (std430, binding = 0) restrict readonly
buffer ModelBlock {
    mat4 models[];
};



void main() {
    mat4 model = models[gl_InstanceID];

    tex_coords  = in_tex_coords;
    // FIXME: do on cpu?
    // Will have to transfer a whole other SSBO.
    normal      = mat3(transpose(inverse(model))) * in_normal;
    frag_pos    = vec3(model * vec4(in_pos, 1.0));
    gl_Position = camera.projview * vec4(frag_pos, 1.0);

}
