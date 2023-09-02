#version 430 core

in vec2 tex_coords;

out vec4 frag_color;


struct AmbientLight {
    vec3 color;
};

struct DirectionalLight {
    vec3 color;
    vec3 direction;
};


struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct PointLight {
    vec3 color;
    vec3 position;
    Attenuation attenuation;
};



uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_albedo_spec;

uniform AmbientLight ambient_light;
uniform DirectionalLight dir_light;


layout (std430, binding = 1) restrict readonly buffer point_light_with_shadows_block {
    PointLight point_lights_with_shadows[];
};

layout (std430, binding = 2) restrict readonly buffer point_light_no_shadows_block {
    PointLight point_lights_no_shadows[];
};


struct CascadeParams {
    mat4 projview;
    float z_split;
};


layout (std430, binding = 3) restrict readonly buffer CascadeParamsBlock {
    CascadeParams cascade_params[];
};

struct DirShadow {
    sampler2DArrayShadow cascades;
    vec2  bias_bounds;
    bool  do_cast;
    int   pcf_extent;
    float pcf_offset;
};


struct PointShadow {
    samplerCubeArrayShadow maps;
    vec2  bias_bounds;
    float z_far;
    int   pcf_extent;
    float pcf_offset;
};


uniform DirShadow dir_shadow;
uniform PointShadow point_shadow;


uniform vec3 cam_pos;








float point_light_shadow_yield(vec3 frag_pos, vec3 normal,
    PointLight plight, int shadow_map_idx);




vec3 add_point_light_illumination(vec3 in_color, vec3 frag_pos,
    float illumination_factor, PointLight plight,
    vec3 tex_diffuse, float tex_specular,
    vec3 normal_dir, vec3 view_dir);


float dir_light_shadow_yield(vec3 frag_pos, vec3 normal);




bool should_discard() {
    // Exit early if no pixels were written in GBuffer.
    return texture(tex_position_draw, tex_coords).a == 0.0;
}


void main() {

    if (should_discard()) discard;

    // Unpack GBuffer.
    vec3  tex_diffuse  = texture(tex_albedo_spec, tex_coords).rgb;
    float tex_specular = texture(tex_albedo_spec, tex_coords).a;
    vec3  frag_pos     = texture(tex_position_draw, tex_coords).rgb;
    vec3  normal       = texture(tex_normals, tex_coords).rgb;


    // Apply lighting and shadows.
    vec3 normal_dir = normalize(normal);
    vec3 view_dir   = normalize(cam_pos - frag_pos);

    vec3 result_color = vec3(0.0);

    // Point lights.
    {
        // With shadows.
        for (int i = 0; i < point_lights_with_shadows.length(); ++i) {
            const PointLight plight = point_lights_with_shadows[i];
            const float illumination_factor =
                1.0 - point_light_shadow_yield(frag_pos, normal, plight, i);

            // How bad is this branching?
            // Is it better to branch or to factor out the result?
            if (illumination_factor == 0.0) continue;

            result_color = add_point_light_illumination(
                result_color, frag_pos,
                illumination_factor, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }

        // Without shadows.
        for (int i = 0; i < point_lights_no_shadows.length(); ++i) {
            const PointLight plight = point_lights_no_shadows[i];

            result_color = add_point_light_illumination(
                result_color, frag_pos,
                1.0, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }
    }

    // Directional light.
    {
        const vec3 light_dir = normalize(dir_light.direction);
        const vec3 reflection_dir = reflect(light_dir, normal_dir);

        const float diffuse_alignment =
            max(dot(normal_dir, -light_dir), 0.0);

        const float specular_alignment =
            max(dot(view_dir, reflection_dir), 0.0);

        const float diffuse_strength = diffuse_alignment;

        // FIXME: Hardcoded shininess because it is not stored
        // in the GBuffer.
        const float specular_strength =
            pow(specular_alignment, 128.0);

        const float illumination_factor =
            dir_shadow.do_cast ?
                (1.0 - dir_light_shadow_yield(frag_pos, normal)) : 1.0;

        result_color +=
            dir_light.color * diffuse_strength *
            tex_diffuse * illumination_factor;

        result_color +=
            dir_light.color * specular_strength *
            tex_specular.r * illumination_factor;
    }


    // Ambient light.
    result_color += ambient_light.color * tex_diffuse;

    frag_color = vec4(result_color, 1.0);

}




float point_light_shadow_yield(vec3 frag_pos, vec3 normal,
    PointLight plight, int shadow_map_idx)
{
    const vec3  light_to_frag = frag_pos - plight.position;
    const float frag_depth    = length(light_to_frag);


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
        point_shadow.bias_bounds[1],
        point_shadow.bias_bounds[0],
        alignment // More aligned - less bias
    ) * frag_depth;


    // Do PCF sampling.

    float yield = 0.0;

    const int   pcf_extent = point_shadow.pcf_extent;
    const int   pcf_order  = 1 + 2 * pcf_extent;
    const float pcf_offset = point_shadow.pcf_offset;


    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {
            for (int z = -pcf_extent; z <= pcf_extent; ++z) {

                const float reference_depth =
                    (frag_depth - total_bias) / point_shadow.z_far;

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
                    point_shadow.maps,
                    vec4(sample_dir, shadow_map_idx),
                    reference_depth
                ).r;

                yield += (1.0 - pcf_lit);
            }
        }
    }

    return yield / (pcf_order * pcf_order * pcf_order);
}





float get_distance_factor(float light_distance, Attenuation att) {
    return 1.0 / (
        att.constant +
        att.linear * light_distance +
        att.quadratic * (light_distance * light_distance)
    );
}




vec3 add_point_light_illumination(vec3 in_color, vec3 frag_pos,
    float illumination_factor, PointLight plight,
    vec3 tex_diffuse, float tex_specular,
    vec3 normal_dir, vec3 view_dir)
{
    vec3 light_vec   = plight.position - frag_pos;
    vec3 light_dir   = normalize(light_vec);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float diffuse_alignment  = max(dot(normal_dir, light_dir), 0.0);
    float specular_alignment = max(dot(normal_dir, halfway_dir), 0.0);

    float distance_factor    = get_distance_factor(length(light_vec), plight.attenuation);

    float diffuse_strength   = distance_factor * diffuse_alignment;
    float specular_strength  = distance_factor * pow(specular_alignment, 128.0);

    in_color +=
        plight.color * diffuse_strength *
        tex_diffuse * illumination_factor;
    in_color +=
        plight.color * specular_strength *
        tex_specular * illumination_factor;

    return in_color;
}













bool is_fragment_in_clip(vec3 frag_pos_clip_space, vec3 clip_border_padding) {
    return all(lessThan(abs(frag_pos_clip_space), vec3(1.0) - clip_border_padding));
}


int select_best_cascade(vec3 frag_pos_world_space, vec3 clip_border_padding) {

    const int num_cascades = cascade_params.length();
    // We assume that cascades are stored in order from smallest
    // to largest, so the first match is the smallest projection,
    // that is highest resolution match.
    for (int i = 0; i < num_cascades; ++i) {

        const vec4 frag_pos_clip_space =
            cascade_params[i].projview * vec4(frag_pos_world_space, 1.0);

        if (is_fragment_in_clip(frag_pos_clip_space.xyz, clip_border_padding)) {
            return i;
        }

    }

    return num_cascades - 1;
}




float dir_light_shadow_yield(vec3 frag_pos, vec3 normal) {

    // Extent is (N-1)/2 for an NxN kernel.
    // So a kernel with extent of 0 is just a 1x1 kernel
    // and extent of 1 gives 3x3 kernel, etc.
    //
    // Order is size of N dimension for NxN PCF kernel.
    //
    // Offset is given in relative texel units.
    // Beware, this does not scale well with the cascades.
    // The larger the scale of the cascade, the larger
    // the absolute offset in world-space becomes, making
    // the shadow edge look "blurrier".
    // TODO: Maybe fix this, looks pretty ugly at the cascade splits otherwise.

    const int   pcf_extent = dir_shadow.pcf_extent;
    const int   pcf_order  = 1 + 2 * pcf_extent;
    const float pcf_offset = dir_shadow.pcf_offset;

    const vec2 texel_size = 1.0 / textureSize(dir_shadow.cascades, 0).xy;

    // We pad each cascade map by the (extent * offset) of the PCF kernel
    // in order to eliminate the gaps between cascades, that normally
    // arise due to sampling over the edge of the cascade map.
    //
    // We also add an extra texel_size padding to account for the fact
    // that the shadow sampler already provides us with the 2x2 PCF
    // on a non-configurable texel_scale.
    //
    // The coefficient 2 is here because we pad in clip space which is [-1, 1]
    // but sample in NDC which is [0, 1], so the padding has to be twice as wide.
    const vec2 padding = 2.0 * (pcf_extent * pcf_offset * texel_size + texel_size);
    const vec3 clip_border_padding = vec3(padding.x, padding.y, 0.0);

    const int cascade_id = select_best_cascade(frag_pos, clip_border_padding);

    // TODO: Redundant calculation. Already done in select_best_cascade().
    // Maybe return as an out parameter or struct?
    const vec4 frag_pos_light_space =
        cascade_params[cascade_id].projview * vec4(frag_pos, 1.0);

    // FIXME: Why the perspective divide even exists for an orthographic projection?
    // Should copy-paste code less.

    // Perspective divide to [-1, 1] and then to NDC [0, 1]
    vec3 frag_pos_ndc =
        ((frag_pos_light_space.xyz / frag_pos_light_space.w) + 1.0) / 2.0;

    if (frag_pos_ndc.z > 1.0) {
        return 0.0;
    }

    const float frag_depth = frag_pos_ndc.z;

    // TODO: How does this work out for different cascades?
    const float total_bias = max(
        dir_shadow.bias_bounds[0],
        dir_shadow.bias_bounds[1] *
            (1.0 - dot(normal, normalize(dir_light.direction)))
    );


    // Do PCF sampling.

    float yield = 0.0;

    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {

            const float reference_depth = frag_depth - total_bias;

            const vec2 offset = vec2(x, y) * pcf_offset * texel_size;

            const vec4 lookup = vec4(
                frag_pos_ndc.xy + offset, // Actual coordinates
                cascade_id,               // Texture index
                reference_depth           // Reference value for shadow-testing
            );

            const float pcf_lit =
                texture(dir_shadow.cascades, lookup).r;

            // Shadow sampler returns "how much of the fragment is lit"
            // from GL_LESS comparison function.
            // We need "how much of the fragment is in shadow".
            yield += (1.0 - pcf_lit);
        }
    }

    // Normalize and return.
    return yield / (pcf_order * pcf_order);
}


