#version 330 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_tex_coords;

out vec2 tex_coords;

uniform float time;

void main() {
    tex_coords = in_tex_coords;

    const float strength = 0.01;
    const float speed = 30.0;
    const float speed_xy_ratio = 1.5;


    gl_Position = vec4(in_pos, 0.0, 1.0);
    gl_Position.x += strength * cos(time * speed);
    gl_Position.y += strength * cos(time * speed * speed_xy_ratio);

}
