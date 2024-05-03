#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"
#include "utils.random.glsl"
#include "camera_ubo.glsl"


in  vec2  tex_coords;
out float frag_color;


uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_depth;

uniform sampler2D tex_noise;
uniform vec2      noise_size; // vec2(0.1, 0.1) for a 16x16 texture on a 160x160 screen

uniform float radius;
uniform float bias;

layout (std430, binding = 0) restrict readonly
buffer KernelSamplesBlock {
    vec4 kernel_samples[];
};


/*
enum class NoiseMode : GLint {
    sampled_from_texture = 0,
    generated_in_shader  = 1
} noise_mode;
*/
const   int noise_mode_sampled   = 0;
const   int noise_mode_generated = 1;
uniform int noise_mode = noise_mode_generated;

/*
enum class PositionSource : GLint {
    gbuffer = 0,
    depth   = 1
} position_source;
*/
const   int position_source_gbuffer = 0;
const   int position_source_depth   = 1;
uniform int position_source = position_source_gbuffer;


vec3 get_random_vector();
mat3 get_random_tbn_matrix(vec3 normal);

vec4 get_frag_pos_vs(vec2 uv);
vec3 get_frag_normal_vs(vec2 uv);




void main() {

    vec4 frag_pos_vs    = get_frag_pos_vs(tex_coords);
    if (any(isinf(frag_pos_vs))) discard;
    vec3 frag_normal_vs = get_frag_normal_vs(tex_coords);
    mat3 tbn_vs         = get_random_tbn_matrix(frag_normal_vs);


    int num_samples = kernel_samples.length();
    float occlusion = 0.0;
    for (int i = 0; i < num_samples; ++i) {
        vec3 sample_pos_ts  = kernel_samples[i].xyz;
        vec4 sample_pos_vs  = vec4((tbn_vs * (radius * sample_pos_ts)) + frag_pos_vs.xyz, 1.0);
        vec3 sample_pos_nss = vs_to_nss(sample_pos_vs, camera.proj);

        vec4 reference_pos_vs = get_frag_pos_vs(sample_pos_nss.xy);

        // TODO: Who are you?
        float range_correction = smoothstep(
            0.0, 1.0,
            radius / abs(reference_pos_vs.z - sample_pos_vs.z)
        );

        occlusion +=
            float(reference_pos_vs.z >= sample_pos_vs.z + bias) * range_correction;
    }

    frag_color = occlusion / float(num_samples);
}




vec4 get_frag_pos_vs(vec2 uv) {
    switch (position_source) {
        case position_source_depth:
        {
            float screen_z = textureLod(tex_depth, uv, 0).r;
            vec3  pos_nss  = vec3(uv, screen_z);
            vec4  pos_vs   = nss_to_vs(pos_nss, camera.z_near, camera.z_far, camera.inv_proj);
            const float far_plane_correction = (1.0 - step(1.0, screen_z)); // Clamp to INF at z_far
            return pos_vs / far_plane_correction;
        }
        default:
        case position_source_gbuffer: // This sucks btw.
        {
            vec4 pos_ws_draw = texture(tex_position_draw, uv);
            // Will return inf if no draw...
            return camera.view * vec4(pos_ws_draw.xyz / pos_ws_draw.a, 1.0);
        }
    }
}


vec3 get_frag_normal_vs(vec2 uv) {
    vec3 frag_normal_ws = texture(tex_normals, uv).xyz;
    return normalize(camera.normal_view * frag_normal_ws);
}





mat3 get_random_tbn_matrix(vec3 normal) {
    vec3 random_vec = get_random_vector();
    vec3 tangent    = normalize(random_vec - dot(random_vec, normal) * normal);
    vec3 bitangent  = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}


vec3 generate_random_vector() {
    uint random_id = uint(gl_FragCoord.y) << 16 | uint(gl_FragCoord.x);
    // FIXME:
    // We commit a sin here where we actually take
    // a box-distributed vector, might be not that bad...
    // Still better than tiling a texture, man.
    float x = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    float y = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    float z = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    return vec3(x, y, z);
}


vec3 sample_random_vector() {
    return texture(tex_noise, tex_coords / noise_size).xyz;
}


vec3 get_random_vector() {
    switch (noise_mode) {
        // There's no performance difference that I notice other
        // than for noise textures of size 1x1 or 2x2 the performance is
        // actually slightly better due to, most likely, more regular
        // access patterns, i.e. being more cache-friendly, since you always
        // sample at the same offset in case of 1x1 for example.
        //
        // Of course, 1x1 noise is not actually noise in any way, so this is
        // practically useless and has the same banding as without noise at all.
        //
        // The visual difference is much more noticable though,
        // the pcg-based generation has no visible tiling on flat surfaces,
        // therefore it's probably a better default.
        case noise_mode_sampled:
            return sample_random_vector();
        default:
        case noise_mode_generated:
            return generate_random_vector();
    }
}
