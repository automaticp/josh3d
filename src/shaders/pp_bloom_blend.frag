#version 330 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D screen_color;
uniform sampler2D bloom_color;

void main() {
    vec3 frag_screen = texture(screen_color, tex_coords).rgb;
    vec3 frag_bloom = texture(bloom_color, tex_coords).rgb;

    frag_color = vec4(frag_screen + frag_bloom, 1.0);
}
