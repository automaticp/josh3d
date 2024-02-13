#version 430 core

in  vec2  tex_coords;
out float frag_color;


uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;

uniform mat4  view;
uniform mat4  proj;
uniform float z_near;
uniform float z_far;

uniform float radius;
uniform float bias;

layout (std430, binding = 0) restrict readonly buffer KernelSamplesBlock {
    vec4 kernel_samples[];
};




mat3 get_random_tbn_matrix(vec3 normal);


// ws - world space,
// ts - tangent space,
// vs - view space,
// cs - clip space,
// ndc - normalized device coordinates
// nss - normalized screen space (ndc/2 + 1/2) (in [0, 1], for texture sampling)
void main() {

    vec4 pos_draw = texture(tex_position_draw, tex_coords);

    if (pos_draw.a == 0.0) discard;

    vec3 frag_pos_ws    = pos_draw.xyz;
    vec3 frag_normal_ws = texture(tex_normals, tex_coords).xyz;

    mat3 tbn = get_random_tbn_matrix(frag_normal_ws);

    int num_samples = kernel_samples.length();
    float occlusion = 0.0;
    for (int i = 0; i < num_samples; ++i) {
        vec3 sample_pos_ts  = kernel_samples[i].xyz;
        vec4 sample_pos_ws  = vec4((tbn * (radius * sample_pos_ts)) + frag_pos_ws, 1.0);
        vec4 sample_pos_vs  = view * sample_pos_ws;
        vec4 sample_pos_cs  = proj * sample_pos_vs;
        vec3 sample_pos_ndc = sample_pos_cs.xyz / sample_pos_cs.w;
        vec3 sample_pos_nss = sample_pos_ndc * 0.5 + 0.5;

        vec4 ref_pos_draw = texture(tex_position_draw, sample_pos_nss.xy);
        // FIXME: Kinda scuffed? Any better way?
        if (ref_pos_draw.a == 0.0) continue;

        vec4 reference_pos_ws = vec4(ref_pos_draw.xyz, 1.0);
        vec4 reference_pos_vs = view * reference_pos_ws;

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




vec3 get_random_vector();


mat3 get_random_tbn_matrix(vec3 normal) {
    vec3 random_vec = get_random_vector();
    vec3 tangent    = normalize(random_vec - dot(random_vec, normal) * normal);
    vec3 bitangent  = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}



// Haha, this is fucking expensive, damn...
uint pcg32(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return result;
}


float gamma(uint urb) {
    return uintBitsToFloat(urb >> 9 | 0x3F800000) - 1.0;
}


vec3 get_random_vector() {
    uint random_id = uint(gl_FragCoord.y) << 16 | uint(gl_FragCoord.x);
    // FIXME:
    // We commit a sin here where we actually take
    // a box-distributed vector, might be not that bad...
    float x = gamma(pcg32(random_id)) * 2.0 - 1.0;
    float y = gamma(pcg32(random_id)) * 2.0 - 1.0;
    float z = gamma(pcg32(random_id)) * 2.0 - 1.0;
    return vec3(x, y, z);
}
