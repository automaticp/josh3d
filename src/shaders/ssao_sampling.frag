#version 430 core

in  vec2  tex_coords;
out float frag_color;


uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_depth;

uniform sampler2D tex_noise;
uniform vec2      noise_size; // vec2(0.1, 0.1) for a 16x16 texture on a 160x160 screen

uniform mat4  view;
uniform mat3  normal_view;
uniform mat4  proj;
uniform mat4  inv_proj;
uniform float z_near;
uniform float z_far;

uniform float radius;
uniform float bias;

layout (std430, binding = 0) restrict readonly buffer KernelSamplesBlock {
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


// ws - world space,
// ts - tangent space,
// vs - view space,
// cs - clip/projection space,
// ndc - normalized device coordinates
// nss - normalized screen space (ndc/2 + 1/2) (in [0, 1], for texture sampling)
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
        vec4 sample_pos_cs  = proj * sample_pos_vs;
        vec3 sample_pos_ndc = sample_pos_cs.xyz / sample_pos_cs.w;
        vec3 sample_pos_nss = sample_pos_ndc * 0.5 + 0.5;

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




float get_vs_depth(float screen_z) {
    return (z_near * z_far) /
        (z_far - screen_z * (z_far - z_near));
}


vec4 get_frag_pos_vs(vec2 uv) {
    switch (position_source) {
        case position_source_depth:
        {
            float screen_z = texture(tex_depth, uv).r;
            float clip_w   = get_vs_depth(screen_z);
            vec4  pos_nss  = vec4(vec3(uv, screen_z), 1.0);
            vec4  pos_ndc  = pos_nss * 2.0 - 1.0;
            vec4  pos_cs   = pos_ndc * clip_w;
            vec4  pos_vs   = inv_proj * pos_cs;
            const float far_plane_correction = (1.0 - step(1.0, screen_z)); // Clamp to INF at z_far
            return pos_vs / far_plane_correction;
        }
        default:
        case position_source_gbuffer:
        {
            vec4 pos_ws_draw = texture(tex_position_draw, uv);
            // Will return inf if no draw...
            return view * vec4(pos_ws_draw.xyz / pos_ws_draw.a, 1.0);
        }
    }
}


vec3 get_frag_normal_vs(vec2 uv) {
    vec3 frag_normal_ws = texture(tex_normals, uv).xyz;
    return normalize(normal_view * frag_normal_ws);
}





mat3 get_random_tbn_matrix(vec3 normal) {
    vec3 random_vec = get_random_vector();
    vec3 tangent    = normalize(random_vec - dot(random_vec, normal) * normal);
    vec3 bitangent  = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}




uint pcg32(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return result;
}


float gamma(uint urb) {
    return uintBitsToFloat(urb >> 9 | 0x3F800000) - 1.0;
}


vec3 generate_random_vector() {
    uint random_id = uint(gl_FragCoord.y) << 16 | uint(gl_FragCoord.x);
    // FIXME:
    // We commit a sin here where we actually take
    // a box-distributed vector, might be not that bad...
    // Still better than tiling a texture, man.
    float x = gamma(pcg32(random_id)) * 2.0 - 1.0;
    float y = gamma(pcg32(random_id)) * 2.0 - 1.0;
    float z = gamma(pcg32(random_id)) * 2.0 - 1.0;
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
