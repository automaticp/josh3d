#version 430 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D color;

uniform vec2  value_range;
uniform float exposure_factor;




/*
Average screen value from previous frame.
*/
layout (std430, binding = 1) restrict readonly
buffer CurrentValueBuffer {
    float screen_value;
} current;


float exposure_function(float screen_value) {
    return exposure_factor / (screen_value + 0.0001);
}


void main() {
    vec3 hdr_color = texture(color, tex_coords).rgb;

    const float min_value = value_range[0];
    const float max_value = value_range[1];
    // TODO: Should this be done in the reduce pass instead?
    // TODO: Could be smoother?
    const float value    = clamp(current.screen_value, min_value, max_value);
    const float exposure = exposure_function(value);

    const vec3 tonemapped_color = vec3(1.0) - exp(-exposure * hdr_color);

    frag_color = vec4(tonemapped_color, 1.0);
}
