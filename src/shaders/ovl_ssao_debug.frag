#version 330 core

in vec2 uv;

uniform sampler2D tex_backbuffer;
uniform sampler2D tex_occlusion;

/*
enum class OverlayMode
{
    None      = 0,
    Back      = 1,
    Occlusion = 2,
};
*/
uniform int mode = 0;

out vec4 frag_color;


void main()
{
    const float inv_gamma = 1.0 / 2.2;

    float out_value;
    switch (mode)
    {
        case 1: // Backbuffer.
            out_value = pow(1.0 - texture(tex_backbuffer, uv).r, inv_gamma);
            break;
        case 2: // Final Occlusion.
        default:
            out_value = pow(1.0 - texture(tex_occlusion, uv).r, inv_gamma);
            break;
    }

    frag_color = vec4(vec3(out_value), 1.0);
}
