#version 330 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D screen_color;
uniform float threshold;

float color_value(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 threshold_sample(vec2 coords, float threshold) {
    vec3 this_color = texture(screen_color, coords).rgb;
    return color_value(this_color) > threshold ? this_color : vec3(0.0);
}


void main() {
    vec3 result_color = threshold_sample(tex_coords, threshold);
    frag_color = vec4(result_color, 1.0);
}
