#version 330 core

in vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D image;
uniform vec3 color;

void main() {
    frag_color = vec4(color, 1.0) * texture(image, tex_coords);
}
