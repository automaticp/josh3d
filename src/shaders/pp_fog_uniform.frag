#version 430 core

in  vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D depth;

uniform vec3  fog_color;

uniform float z_near;
uniform float z_far;
uniform mat4  inv_proj;

uniform float mean_free_path;
uniform float distance_power;
uniform float cutoff_offset;



// Exponent-based visibility simulating const density (physical) fog
// or power-corrected (!= 1.0) fog (non-physical).
float transmittance(float frag_distance) {
    return exp(-pow(frag_distance / mean_free_path, distance_power));
}


// Smoothly maximizes the fog factor when close to the z_far.
// This way the fog factor for background objects is not as dependant
// on z_far. Of course, you will not see the background at all.
float tail(float frag_distance) {
    float z_cutoff_start = max(z_far - cutoff_offset, z_near);
    return 1.0 - smoothstep(z_cutoff_start, z_far, frag_distance);
}


float get_view_space_depth(float screen_depth) {
    // Linearization back from non-linear depth encoding:
    //   screen_depth = (1/z - 1/n) / (1/f - 1/n)
    // Where z, n and f are: view-space z, z_near and z_far respectively.
    return (z_near * z_far) /
        (z_far - screen_depth * (z_far - z_near));
}


void main() {

    float screen_depth = texture(depth, tex_coords).r;
    float view_depth   = get_view_space_depth(screen_depth);

    float w_clip    = view_depth;
    vec4  pos_ndc   = vec4(vec3(tex_coords, screen_depth) * 2.0 - 1.0, 1.0);
    vec4  pos_clip  = pos_ndc * w_clip;
    vec4  pos_view  = inv_proj * pos_clip;

    float distance_to_frag = length(pos_view.xyz);

    float transmittance_factor = transmittance(distance_to_frag);
    float tail_factor          = tail(distance_to_frag);

    float fog_factor = 1.0 - transmittance_factor * tail_factor;

    // And then just blend.
    frag_color = vec4(fog_color, fog_factor);

    // For debug, fog only:
    // frag_color = vec4(fog_color * vec3(fog_factor), 1.0);
}
