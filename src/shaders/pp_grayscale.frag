#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D color;

void main() {
    frag_color = texture(color, tex_coords);
    const vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float avg = dot(weights, frag_color.rgb);
    frag_color = vec4(vec3(avg), 1.0);
}
