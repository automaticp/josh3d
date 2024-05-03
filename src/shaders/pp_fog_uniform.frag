#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"
#include "camera_ubo.glsl"


in  vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D depth;
uniform vec3      fog_color;
uniform float     mean_free_path;
uniform float     distance_power;
uniform float     cutoff_offset;



// Exponent-based visibility simulating const density (physical) fog
// or power-corrected (!= 1.0) fog (non-physical).
float transmittance(float frag_distance) {
    return exp(-pow(frag_distance / mean_free_path, distance_power));
}


// Smoothly maximizes the fog factor when close to the z_far.
// This way the fog factor for background objects is not as dependant
// on z_far. Of course, you will not see the background at all.
float tail(float frag_distance) {
    float z_cutoff_start = max(camera.z_far - cutoff_offset, camera.z_near);
    return 1.0 - smoothstep(z_cutoff_start, camera.z_far, frag_distance);
}




void main() {

    float screen_z = texture(depth, tex_coords).r;
    vec3 pos_nss   = vec3(tex_coords, screen_z);
    vec4 pos_vs    = nss_to_vs(pos_nss, camera.z_near, camera.z_far, camera.inv_proj);

    float distance_to_frag = length(pos_vs.xyz);

    float transmittance_factor = transmittance(distance_to_frag);
    float tail_factor          = tail(distance_to_frag);

    float fog_factor = 1.0 - transmittance_factor * tail_factor;

    // And then just blend.
    frag_color = vec4(fog_color, fog_factor);

    // For debug, fog only:
    // frag_color = vec4(fog_color * vec3(fog_factor), 1.0);
}
