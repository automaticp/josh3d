#version 330 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_tex_coords;

out vec2 tex_coords;

void main() {
    tex_coords = in_tex_coords;
    // Clip at z-far with z = 1.0.
    gl_Position = vec4(in_pos.x, in_pos.y, 1.0, 1.0);
}


