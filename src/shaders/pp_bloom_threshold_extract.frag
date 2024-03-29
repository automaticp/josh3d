#version 330 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D screen_color;

uniform vec2 threshold_bounds;



float color_value(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 threshold_sample(vec2 coords) {
    vec3 this_color = texture(screen_color, coords).rgb;

    float color_factor = // Between 0 and 1
        smoothstep(threshold_bounds[0], threshold_bounds[1], color_value(this_color));

    return color_factor * this_color;
}


void main() {
    vec3 result_color = threshold_sample(tex_coords);
    frag_color = vec4(result_color, 1.0);
}
