#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.color.glsl"


in vec2  tex_coords;
out vec4 frag_color;

uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;

uniform struct DirectionalLight {
    vec3 color;
    vec3 direction;
} dir_light;


struct CascadeParams {
    mat4  projview;
    vec3  scale;
    float z_split;
};

layout (std430, binding = 3) restrict readonly buffer CascadeParamsBlock {
    CascadeParams cascade_params[];
};




float split_hue(float hmin, float hmax, int split_id, int num_splits) {
    const float hstep = (hmax - hmin) / num_splits;
    return hmin + hstep * split_id;
}


bool should_discard() {
    return texture(tex_position_draw, tex_coords).a == 0.0;
}


bool is_fragment_inside_clip(vec3 frag_pos_clip_space) {
    return all(lessThanEqual(abs(frag_pos_clip_space), vec3(1.0)));
}

// With "inspiration" from
// https://github.com/wessles/vkmerc/blob/cb087f425cdbc0b204298833474cf62874505388/examples/res/shaders/misc/cascade_debugger.frag
void main() {
    if (should_discard()) discard;

    vec3 frag_pos = texture(tex_position_draw, tex_coords).rgb;
    vec3 normal   = texture(tex_normals, tex_coords).rgb;

    const float diffuse_alignment =
        max(dot(normalize(normal), -dir_light.direction), 0.0);

    vec3 result_color = 0.1 + 0.9 * vec3(diffuse_alignment);

    const float hue_min = 0.0;
    const float hue_max = 1.0;

    const int num_cascades = cascade_params.length();

    // The assumption is that cascades are stored from smallest to largest.
    // So we iterate forwards to test smaller cascades first.
    for (int cid = 0; cid < num_cascades; ++cid) {

        const vec3 color = hsv2rgb(vec3(split_hue(hue_min, hue_max, cid, num_cascades), 1.0, 1.0));

        const vec4 frag_pos_light_space =
            cascade_params[cid].projview * vec4(frag_pos, 1.0);

        if (is_fragment_inside_clip(frag_pos_light_space.xyz)) {
            result_color *= color;
            break;
        }
    }

    const float inv_gamma = 1.0 / 2.2;
    frag_color = vec4(pow(result_color, vec3(inv_gamma)), 1.0);
}
