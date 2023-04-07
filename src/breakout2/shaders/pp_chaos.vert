#version 330 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_tex_coords;

out vec2 tex_coords;

uniform float time;

void main() {
    const float strength = 0.3;
    const float speed = 1.0;
    const float speed_xy_ratio = 1.0;

    vec2 offsets = strength * vec2(sin(time * speed), cos(time * speed * speed_xy_ratio));

    tex_coords = in_tex_coords + offsets;

    gl_Position = vec4(in_pos, 0.0, 1.0);
}
