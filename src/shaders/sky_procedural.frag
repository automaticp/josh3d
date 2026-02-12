#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

in vec2 uv; // Just screen coordinates in [0, 1]

uniform vec3  light_dir_view_space;
uniform vec3  sky_color;
uniform vec3  sun_color;
uniform float sun_size_rad;

out vec4 frag_color;


void main()
{
    vec3 out_color;

    if (sun_size_rad != 0.0)
    {
        // screen_depth is always 1.0, other fragments are discarded prior.
        const float screen_depth = 1.0;
        const float view_depth   = camera.z_far;

        const float w_clip   = view_depth;
        const vec4  pos_ndc  = vec4(vec3(uv, screen_depth) * 2.0 - 1.0, 1.0);
        const vec4  pos_clip = pos_ndc * w_clip;
        const vec4  pos_view = camera.inv_proj * pos_clip;
        const vec3  frag_dir_view_space = normalize(pos_view.xyz);

        out_color = mix(
            sky_color,
            sun_color,
            step(cos(2.0 * sun_size_rad), dot(-frag_dir_view_space, light_dir_view_space))
        );
    }
    else
    {
        out_color = sky_color;
    }

    frag_color = vec4(out_color, 1.0);
}


