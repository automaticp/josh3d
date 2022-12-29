#version 330 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_tex_coord;

out vec2 tex_coords;

uniform mat4 model;
uniform mat4 projection;

void main() {
    tex_coords = in_tex_coord;
    gl_Position = projection * model * vec4(in_pos, 0.0, 1.0);
}

