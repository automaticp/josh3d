#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D color;
uniform float gamma;

void main() {
    frag_color = pow(texture(color, tex_coords), vec4(1.0 / gamma));
}
