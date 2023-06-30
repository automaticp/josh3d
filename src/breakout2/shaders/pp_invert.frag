#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D color;


void main() {
    frag_color = texture(color, tex_coords);
    frag_color = vec4(1.0 - frag_color.rgb, 1.0);
}
