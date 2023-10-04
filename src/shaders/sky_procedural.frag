#version 330 core

in vec2 tex_coords; // Just screen coordinates in [0, 1]

out vec4 frag_color;


uniform float z_far;
uniform mat4  inv_proj;
uniform vec3  light_dir_view_space;

uniform vec3  sky_color;
uniform vec3  sun_color;
uniform float sun_size_rad;

void main() {

    // screen_depth is always 1.0, other fragments are discarded prior.
    float screen_depth = 1.0;
    float view_depth   = z_far;

    float w_clip   = view_depth;
    vec4  pos_ndc  = vec4(vec3(tex_coords, screen_depth) * 2.0 - 1.0, 1.0);
    vec4  pos_clip = pos_ndc * w_clip;
    vec4  pos_view = inv_proj * pos_clip;
    vec3  frag_dir_view_space = normalize(pos_view.xyz);

    vec3 out_color = mix(
        sky_color,
        sun_color,
        step(cos(2.0 * sun_size_rad), dot(-frag_dir_view_space, light_dir_view_space))
    );

    frag_color = vec4(out_color, 1.0);
}


