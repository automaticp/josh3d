#version 330 core

in  vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D tex_noisy_occlusion;
uniform sampler2D tex_occlusion;

/*
enum class OverlayMode : GLint {
    none    = 0,
    noisy   = 1,
    blurred = 2,
};
*/
uniform int mode = 0;




void main() {
    const float inv_gamma = 1.0 / 2.2;

    float out_value;
    switch (mode) {
        case 1: // noisy
            out_value = pow(1.0 - texture(tex_noisy_occlusion, tex_coords).r, inv_gamma);
            break;
        case 2: // blurred
        default:
            out_value = pow(1.0 - texture(tex_occlusion, tex_coords).r, inv_gamma);
            break;
    }

    frag_color = vec4(vec3(out_value), 1.0);
}
