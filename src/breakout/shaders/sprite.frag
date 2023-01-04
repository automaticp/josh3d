#version 330 core

in vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D image;
uniform vec4 color;

void main() {
    frag_color = color * texture(image, tex_coords);
}
