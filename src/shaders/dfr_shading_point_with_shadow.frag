#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "gbuffer.glsl"
#include "camera_ubo.glsl"


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




struct PointShadows {
    samplerCubeArrayShadow maps;
    vec2  bias_bounds;
    int   pcf_extent;
    float pcf_offset;
};




uniform GBuffer      gbuffer;
uniform PointShadows point_shadows;
uniform float        fade_start_fraction;
uniform float        fade_length_fraction;




float point_light_shadow_obscurance(
    PointShadows point_shadows,
    uint         plight_id,
    vec3         light_to_frag,
    vec3         normal,
    float        z_far);


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
    const vec3 view_dir   = normalize(camera.position - gsample.position);

    const PLight plight      = in_.plight;
    const vec3 light_to_frag = gsample.position - plight.position;


    vec3 result_color = vec3(0.0);

    // Point Light.
    {
        const float obscurance =
            point_light_shadow_obscurance(
                point_shadows,
                in_.plight_id,
                light_to_frag,
                gsample.normal,
                in_.plight.radius
            );

        add_point_light_illumination(
            result_color,
            plight,
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




float point_light_shadow_obscurance(
    PointShadows point_shadows,
    uint         plight_id,
    vec3         light_to_frag,
    vec3         normal,
    float        z_far)
{
    const float frag_depth = length(light_to_frag);

    // This bias model offsets the surface along the normal
    // depending on the alignment between the light-to-frag direction
    // and the surface normal. This causes some issues alongside the
    // PCF since the offset PCF samples might require higher bias
    // then the one precalculated for the central fragment.
    //
    // Hypothetically, knowing the offsets, we should be able to precalculate
    // the bias for each of the PCF samples under the assumption
    // that the surface is planar and it's normal doesn't change.
    //
    // Worst case, we can resample the normals at each screen pixel
    // using the G-Buffer, but that involves a lot of transformations
    // and NxN more samples. Probably not worth it.
    const float alignment  = dot(normal, -normalize(light_to_frag));
    const float total_bias = mix(
        point_shadows.bias_bounds[1],
        point_shadows.bias_bounds[0],
        alignment // More aligned - less bias
    ) * frag_depth;


    // Do PCF sampling.

    float obscurance = 0.0;

    const int   pcf_extent = point_shadows.pcf_extent;
    const int   pcf_order  = 1 + 2 * pcf_extent;
    const float pcf_offset = point_shadows.pcf_offset;


    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {
            for (int z = -pcf_extent; z <= pcf_extent; ++z) {

                const float reference_depth =
                    (frag_depth - total_bias) / z_far;

                // The "angular" offset of the offset direction depends
                // on the light-to-frag distance due to the unnormalized
                // addition. This preserves the visible PCF area across
                // all distances. The alternative, where light_to_frag
                // is normalized, is way, way worse, as the PCF area
                // grows rapidly with distance from the light source,
                // requiring more filtering to look smooth.
                const vec3 sample_dir = light_to_frag +
                    pcf_offset * vec3(x, y, z) / pcf_extent;

                // Sample from normalized depth value in [0, 1].
                float pcf_lit = texture(
                    point_shadows.maps,
                    vec4(sample_dir, plight_id),
                    reference_depth
                ).r;

                obscurance += (1.0 - pcf_lit);
            }
        }
    }

    return obscurance / (pcf_order * pcf_order * pcf_order);
}
