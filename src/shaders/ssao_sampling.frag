#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"
#include "utils.random.glsl"
#include "camera_ubo.glsl"

in vec2 uv;

layout (std430, binding = 0) restrict readonly
buffer KernelSamplesBlock
{
    vec4 kernel_samples[];
};

uniform sampler2D tex_depth;
uniform sampler2D tex_normals;
uniform sampler2D tex_noise;
uniform vec2      noise_size; // Ex. vec2(0.1, 0.1) for a 16x16 texture on a 160x160 screen

uniform float radius;
uniform float bias;

/*
enum class NoiseMode : i32
{
    sampled_from_texture = 0,
    generated_in_shader  = 1
};
*/
const   int noise_mode_sampled   = 0;
const   int noise_mode_generated = 1;
uniform int noise_mode = noise_mode_generated;

out float frag_color;


vec3 get_random_vector();
mat3 get_random_tbn_matrix(vec3 normal);

vec4 get_frag_pos_vs(vec2 uv);
vec3 get_frag_normal_vs(vec2 uv);

void main()
{
    const vec4 frag_pos_vs    = get_frag_pos_vs(uv);
    if (any(isinf(frag_pos_vs))) discard;
    const vec3 frag_normal_vs = get_frag_normal_vs(uv);
    const mat3 tbn_vs         = get_random_tbn_matrix(frag_normal_vs);

    float occlusion = 0.0;
    const int num_samples = kernel_samples.length();
    for (int i = 0; i < num_samples; ++i)
    {
        const vec3 sample_pos_ts  = kernel_samples[i].xyz;
        const vec4 sample_pos_vs  = vec4((tbn_vs * (radius * sample_pos_ts)) + frag_pos_vs.xyz, 1.0);
        const vec3 sample_pos_nss = vs_to_nss(sample_pos_vs, camera.proj);

        const vec4 reference_pos_vs = get_frag_pos_vs(sample_pos_nss.xy);

        // TODO: Who are you?
        const float range_correction = smoothstep(0.0, 1.0,
            radius / abs(reference_pos_vs.z - sample_pos_vs.z));

        occlusion +=
            float(reference_pos_vs.z >= sample_pos_vs.z + bias) * range_correction;
    }

    frag_color = occlusion / float(num_samples);
}


vec4 get_frag_pos_vs(vec2 uv)
{
    const float screen_z = textureLod(tex_depth, uv, 0).r;
    const vec3  pos_nss  = vec3(uv, screen_z);
    const vec4  pos_vs   = nss_to_vs(pos_nss, camera.z_near, camera.z_far, camera.inv_proj);
    const float far_plane_correction = (1.0 - step(1.0, screen_z)); // Clamp to INF at z_far
    return pos_vs / far_plane_correction;
}

vec3 get_frag_normal_vs(vec2 uv)
{
    const vec3 frag_normal_ws = texture(tex_normals, uv).xyz;
    return normalize(camera.normal_view * frag_normal_ws);
}

mat3 get_random_tbn_matrix(vec3 normal)
{
    const vec3 random_vec = get_random_vector();
    const vec3 tangent    = normalize(random_vec - dot(random_vec, normal) * normal);
    const vec3 bitangent  = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}

vec3 generate_random_vector()
{
    uint random_id = uint(gl_FragCoord.y) << 16 | uint(gl_FragCoord.x);
    // FIXME:
    // We commit a sin here where we actually take
    // a box-distributed vector, might be not that bad...
    float x = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    float y = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    float z = random_gamma(pcg32(random_id)) * 2.0 - 1.0;
    return vec3(x, y, z);
}

vec3 sample_random_vector()
{
    return texture(tex_noise, uv / noise_size).xyz;
}

vec3 get_random_vector()
{
    switch (noise_mode)
    {
        // There's no performance difference that I notice other
        // than for noise textures of size 1x1 or 2x2 the performance is
        // actually slightly better due to, most likely, more regular
        // access patterns, i.e. being more cache-friendly, since you always
        // sample at the same offset in case of 1x1 for example.
        //
        // Of course, 1x1 noise is not actually noise in any way, so this is
        // practically useless and has the same banding as without noise at all.
        case noise_mode_sampled:
            return sample_random_vector();
        default:
        case noise_mode_generated:
            return generate_random_vector();
    }
}
