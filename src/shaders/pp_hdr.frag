#version 330 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D color;

uniform bool use_reinhard;
uniform bool use_exposure;
uniform float exposure;

void main() {
    vec3 hdr_color = texture(color, tex_coords).rgb;

    if (use_reinhard) {
        hdr_color = hdr_color / (hdr_color + vec3(1.0));
    } else if (use_exposure) {
        hdr_color = vec3(1.0) - exp(-exposure * hdr_color);
    }

    frag_color = vec4(hdr_color, 1.0);
}
