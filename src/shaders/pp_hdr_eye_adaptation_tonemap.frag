#version 430 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D color;

uniform float exposure_factor;

/*
Average screen value from previous frame.
*/
layout (std430, binding = 1) restrict readonly buffer CurrentValueBuffer {
    float screen_value;
} current;


float exposure_function(float screen_value) {
    return exposure_factor / (screen_value + 0.0001);
}


void main() {
    vec3 hdr_color = texture(color, tex_coords).rgb;

    float exposure = exposure_function(current.screen_value);
    vec3 tonemapped_color = vec3(1.0) - exp(-exposure * hdr_color);

    frag_color = vec4(tonemapped_color, 1.0);
}
