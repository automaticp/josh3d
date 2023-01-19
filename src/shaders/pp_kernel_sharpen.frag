#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D color;




vec3[9] sample_at_offset(float offset) {

    const vec2 offset_base[9] = vec2[9](
        vec2(-1.0f,  1.0f), // top-left
        vec2( 0.0f,  1.0f), // top-center
        vec2( 1.0f,  1.0f), // top-right
        vec2(-1.0f,  0.0f), // center-left
        vec2( 0.0f,  0.0f), // center-center
        vec2( 1.0f,  0.0f), // center-right
        vec2(-1.0f, -1.0f), // bottom-left
        vec2( 0.0f, -1.0f), // bottom-center
        vec2( 1.0f, -1.0f)  // bottom-right
    );

    vec3 tex_samples[9];
    for (int i = 0; i < 9; ++i) {
        tex_samples[i] = vec3(
            texture(color, tex_coords.st + offset_base[i] * offset)
        );
    }

    return tex_samples;
}


void main() {
    const float offset = 1.0 / 300.0;

    vec3 samples[9] = sample_at_offset(offset);

    const float kernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );
    const float kernel_factor = 1.0;

    vec3 end_color = vec3(0.0);

    for (int i = 0; i < 9; ++i) {
        end_color += samples[i] * kernel[i] * kernel_factor;
    }

    frag_color = vec4(end_color, 1.0);

}

