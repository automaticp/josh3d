#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

uniform mat4 model;
uniform mat3 normal_model;

out vec2 uv;
out vec3 normal;
out vec3 frag_pos;


void main()
{
    uv          = in_tex_coords;
    normal      = normalize(normal_model * in_normal);
    frag_pos    = vec3(model * vec4(in_pos, 1.0));
    gl_Position = camera.projview * vec4(frag_pos, 1.0);
}

