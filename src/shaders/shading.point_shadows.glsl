// #version 430 core
#ifndef SHADING_POINT_SHADOWS_GLSL
#define SHADING_POINT_SHADOWS_GLSL


struct PointShadowParams {
    vec2  bias_bounds;
    int   pcf_extent;
    float pcf_offset;
};


/*
Fraction [0, 1] of how much a point light source is obscured by shadow.
*/
float point_light_shadow_obscurance(
    samplerCubeArrayShadow shadow_maps,
    int                    shadow_map_idx,
    PointShadowParams      params,
    vec3                   light_to_frag,
    vec3                   normal_dir,
    float                  z_far)
{
    const float frag_depth = length(light_to_frag);

    // This bias model offsets the surface along the normal
    // depending on the alignment between the light-to-frag direction
    // and the surface normal. This causes some issues alongside the
    // PCF since the offset PCF samples might require higher bias
    // then the one precalculated for the central fragment.
    const float alignment = dot(normal_dir, -normalize(light_to_frag));
    const float min_bias  = params.bias_bounds[0];
    const float max_bias  = params.bias_bounds[1];
    const float bias      = mix(max_bias, min_bias, alignment) * frag_depth; // More aligned - less bias


    // Do PCF sampling.
    float obscurance = 0.0;

    const int   pcf_extent = params.pcf_extent;
    const int   pcf_order  = 1 + 2 * pcf_extent;
    const float pcf_offset = params.pcf_offset;


    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {
            for (int z = -pcf_extent; z <= pcf_extent; ++z) {

                const float reference_depth = (frag_depth - bias) / z_far;

                // The "angular" offset of the offset direction depends
                // on the light-to-frag distance due to the unnormalized
                // addition. This preserves the visible PCF area across
                // all distances. The alternative, where light_to_frag
                // is normalized, is way, way worse, as the PCF area
                // grows rapidly with distance from the light source,
                // requiring more filtering to look smooth.
                const vec3 sample_dir = light_to_frag + pcf_offset * vec3(x, y, z) / pcf_extent;

                const vec4 lookup = vec4(sample_dir, shadow_map_idx);

                // Sample from normalized depth value in [0, 1].
                const float pcf_lit = texture(shadow_maps, lookup, reference_depth).r;

                obscurance += (1.0 - pcf_lit);
            }
        }
    }

    return obscurance / (pcf_order * pcf_order * pcf_order);
}


#endif
