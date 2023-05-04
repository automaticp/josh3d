#version 430 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D screen_color;
uniform bool blur_horizontally; // or vertically
uniform float offset_scale; // in pixels/texels?

layout (std430, binding = 0) restrict readonly buffer weights_buffer {
    float weights[];
};


void main() {
    vec2 texel_size = 1.0 / textureSize(screen_color, 0);

    vec2 blur_dir = blur_horizontally ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

    vec3 result_color = vec3(0.0);

    for (int i = 0; i < weights.length(); ++i) {
        int offset = i - weights.length() / 2;

        vec2 sample_coords =
            tex_coords + blur_dir * (offset * offset_scale * texel_size);

        result_color +=
            texture(screen_color, sample_coords).rgb * weights[i];
    }

    frag_color = vec4(result_color, 1.0);
}
