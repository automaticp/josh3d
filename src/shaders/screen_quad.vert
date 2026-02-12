#version 330 core

layout (location = 0) in vec2 in_pos;
layout (location = 2) in vec2 in_uv;

out vec2 uv;


void main()
{
    uv          = in_uv;
    gl_Position = vec4(in_pos.x, in_pos.y, 0.0, 1.0);
}
