#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "gbuffer.glsl"


out vec4 frag_color;


struct PLight {
    vec3        color;
    vec3        position;
    float       radius;
    Attenuation attenuation;
};


in Interface {
    flat uint  plight_id;
    PLight     plight;
} in_;


uniform GBuffer gbuffer;
uniform vec3    cam_pos;
uniform float   fade_start_fraction;
uniform float   fade_length_fraction;




void add_point_light_illumination(
    inout vec3 color,
    PLight     plight,
    float      obscurance,
    vec3       light_to_frag,
    vec3       normal_dir,
    vec3       view_dir,
    vec3       albedo,
    float      specular);




void main() {
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(gbuffer.tex_albedo_spec, 0));

    GSample gsample = unpack_gbuffer(gbuffer, uv);
    if (!gsample.drawn) discard;

    const vec3 normal_dir = normalize(gsample.normal);
    const vec3 view_dir   = normalize(cam_pos - gsample.position);

    const vec3 light_to_frag = gsample.position - in_.plight.position;
    const float obscurance   = 0.0; // No shadow.


    vec3 result_color = vec3(0.0);

    // Point Light.
    {
        add_point_light_illumination(
            result_color,
            in_.plight,
            obscurance,
            light_to_frag,
            normal_dir,
            view_dir,
            gsample.albedo,
            gsample.specular
        );
    }

    // Fade near the light volume edges.
    {
        const float radius        = in_.plight.radius;
        const float frag_distance = length(light_to_frag);
        const float fade_start    = radius * fade_start_fraction;
        const float fade_end      = min(radius, fade_start + radius * fade_length_fraction);
        const float fade_factor   = 1.0 - smoothstep(fade_start, fade_end, frag_distance);

        result_color *= fade_factor;
    }


    frag_color = vec4(result_color, 1.0);
}




void add_point_light_illumination(
    inout vec3 color,
    PLight     plight,
    float      obscurance,
    vec3       light_to_frag,
    vec3       normal_dir,
    vec3       view_dir,
    vec3       albedo,
    float      specular)
{
    const vec3 light_vec   = -light_to_frag;
    const vec3 light_dir   = normalize(light_vec);
    const vec3 halfway_dir = normalize(light_dir + view_dir);

    const float diffuse_alignment  = max(dot(normal_dir, light_dir),   0.0);
    const float specular_alignment = max(dot(normal_dir, halfway_dir), 0.0);

    const float distance_factor = get_attenuation_factor(plight.attenuation, length(light_vec));

    const float diffuse_strength   = distance_factor * diffuse_alignment;
    const float specular_strength  = distance_factor * pow(specular_alignment, 128.0);

    color += plight.color * diffuse_strength  * albedo   * (1.0 - obscurance);
    color += plight.color * specular_strength * specular * (1.0 - obscurance);
}

