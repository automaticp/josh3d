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

uniform sampler2D tex_ambient_occlusion;
uniform bool      use_ambient_occlusion = false;
uniform float     ambient_occlusion_power = 1.0;

uniform AmbientLight ambient_light;
uniform DirectionalLight dir_light;


layout (std430, binding = 1) restrict readonly buffer point_light_with_shadows_block {
    PointLight point_lights_with_shadows[];
};

layout (std430, binding = 2) restrict readonly buffer point_light_no_shadows_block {
    PointLight point_lights_no_shadows[];
};


struct CascadeParams {
    mat4  projview;
    vec3  scale;
    float z_split;
};


layout (std430, binding = 3) restrict readonly buffer CascadeParamsBlock {
    CascadeParams cascade_params[];
};

struct DirShadow {
    sampler2DArrayShadow cascades;
    float base_bias_tx;
    bool  do_blend_cascades;
    float blend_size_inner_tx;
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

    if (use_ambient_occlusion) {
        float ambient_occlusion = texture(tex_ambient_occlusion, tex_coords).r;
        result_color +=
            ambient_light.color * tex_diffuse *
            pow(1.0 - ambient_occlusion, ambient_occlusion_power);
    } else {
        result_color +=
            ambient_light.color * tex_diffuse;
    }




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


struct CascadeSelection {
    int   inner_cascade_id;
    float outer_cascade_blend_factor;
};

CascadeSelection select_best_cascade_blend(
    vec3 frag_pos_world_space, vec3 clip_discard_padding, vec3 clip_blend_padding)
{

    const int num_cascades    = cascade_params.length();
    const int last_cascade_id = num_cascades - 1;
    // We assume that cascades are stored in order from smallest
    // to largest, so the first match is the smallest projection,
    // that is highest resolution match.
    for (int i = 0; i < last_cascade_id; ++i) {

        const vec4 frag_pos_clip_space =
            cascade_params[i].projview * vec4(frag_pos_world_space, 1.0);

        const vec3 total_border_padding = clip_discard_padding + clip_blend_padding;

        if (is_fragment_in_clip(frag_pos_clip_space.xyz, clip_discard_padding)) {
            if (is_fragment_in_clip(frag_pos_clip_space.xyz, total_border_padding)) {
                return CascadeSelection(i, 0.0);
            } else /* Welcome to THE BLEND ZONE!!! */ {
                const vec3 blend_start = vec3(1.0) - total_border_padding;
                const vec3 outer_blend_factors =
                    (abs(frag_pos_clip_space.xyz) - blend_start) / clip_blend_padding;

                // TODO: Maybe return dimensional blend factors?
                const float outer_blend_factor = max(outer_blend_factors.x, outer_blend_factors.y);
                return CascadeSelection(i, outer_blend_factor);
            }
        }

    }

    // If we hit the last cascade or there's only one cascade total,
    // then select it, no padding or blending.
    return CascadeSelection(last_cascade_id, 0.0);
}




int select_best_cascade(
    vec3 frag_pos_world_space, vec3 clip_discard_padding)
{

    const int num_cascades    = cascade_params.length();
    const int last_cascade_id = num_cascades - 1;
    // We assume that cascades are stored in order from smallest
    // to largest, so the first match is the smallest projection,
    // that is highest resolution match.
    for (int i = 0; i < last_cascade_id; ++i) {

        const vec4 frag_pos_clip_space =
            cascade_params[i].projview * vec4(frag_pos_world_space, 1.0);

        if (is_fragment_in_clip(frag_pos_clip_space.xyz, clip_discard_padding)) {
            return i;
        }

    }

    // If we hit the last cascade or there's only one cascade total,
    // then select it, no padding.
    return last_cascade_id;
}





float dir_shadow_yield_pcf_sample(
    int pcf_extent, int pcf_order, vec2 pcf_offset_tx,
    vec2 texel_size, int cascade_id, vec3 normal_light_space, vec3 frag_pos_ndc);




float dir_light_shadow_yield(vec3 frag_pos, vec3 normal) {

    // pcf_extent is (N-1)/2 for an NxN kernel.
    // So a kernel with extent of 0 is just a 1x1 kernel
    // and extent of 1 gives 3x3 kernel, extent of 2 gives 5x5, etc.
    //
    // pcf_order is the size N of a NxN PCF kernel.
    // A 5x5 kernel is of order 5.
    //
    // pcf_offset is given in the number of texels of the innermost cascade.
    // The actual effective PCF area in world-space does not scale with
    // the cascades, it stays constant across all distances.
    // If you want it to scale in a continuous fashion, see the part
    // about offset scaling in code (not example) below.
    //
    // Given an offset of 2.0 (texels) and a 3x3 kernel, the effective
    // filtering area is then:
    //     pcf_area_world = (N * texel_size_world_space * pcf_offset)^2;
    //                    = (3 * texel_size_world_space * 2.0       )^2;
    // where texel_size_world_space can be taken from:
    //     texel_size_world_space = texel_size * cascade_params[cascade_id].scale.xy;
    //
    // TODO:
    // It might make sense to derive the offset from some parameter that defines the
    // effective PCF area instead (like pcf_width or something), so that the area
    // is preserved between different kernel sizes and shadowmap resolutions.
    // I'm just too lazy right now.

    const int   pcf_extent = dir_shadow.pcf_extent;
    const int   pcf_order  = 1 + 2 * pcf_extent;
    const vec2  pcf_offset = vec2(dir_shadow.pcf_offset);

    const vec2  texel_size = 1.0 / textureSize(dir_shadow.cascades, 0).xy;

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
    const vec3 clip_discard_padding = vec3(padding.x, padding.y, 0.0);

    // For blending we use the terms like "inner" and "outer" cascades:
    //   - Inner cascade is just the best resolution cascade available for the fragment;
    //   - Outer cascade is the next best resolution cascade after the inner one.
    int   inner_cascade_id;
    float outer_cascade_blend = 0.0;

    if (dir_shadow.do_blend_cascades) {
        const float blend_size_inner_tx = dir_shadow.blend_size_inner_tx;
        const vec2  blend_padding       = blend_size_inner_tx * texel_size * 0.5;
        const vec3  clip_blend_padding  = vec3(blend_padding.x, blend_padding.y, 0.0);

        const CascadeSelection selection =
            select_best_cascade_blend(frag_pos, clip_discard_padding, clip_blend_padding);

        inner_cascade_id    = selection.inner_cascade_id;
        outer_cascade_blend = selection.outer_cascade_blend_factor;

    } else {
        inner_cascade_id    = select_best_cascade(frag_pos, clip_discard_padding);
    }


    // Everything that follows with biasing and PCF sampling is best done
    // in light space, so we go there.

    // Get the position and normal in light space.
    const mat4 inner_cascade_pv           = cascade_params[inner_cascade_id].projview;
    const vec4 inner_frag_pos_light_space = inner_cascade_pv * vec4(frag_pos, 1.0);

    // All cascades are parallel, so any cascade will work for plane normals.
    const mat3 cascade_normal_pv  = mat3(transpose(inverse(inner_cascade_pv)));
    const vec3 normal_light_space = normalize(cascade_normal_pv * normal);

    // Perspective divide to [-1, 1] (none for orthographic) and then to NDC [0, 1]
    vec3 inner_frag_pos_ndc =
        0.5 * (1.0 + inner_frag_pos_light_space.xyz / inner_frag_pos_light_space.w);

    // FIXME: Still need to fix the bug where the cascades cut-off early.
    if (inner_frag_pos_ndc.z > 1.0) { return 0.0; }


    // We scale the pcf_offset so that it's constant size in world-space across multiple
    // cascades. This eliminates the discrete nature of the visible PCF area and makes the
    // cascade "transition line" practically unnoticable if the aliasing of both cascades
    // is fully filtered out or not visible at this screen-pixel to shadowmap-texel ratio.
    //
    // The aliasing (blocky shadowmap) is going to be present at farther cascades because
    // a good filtering scale for the nearest cascade is not enough for a distant one.
    // Again, since the cascade is distant, you probably won't see the aliasing directly,
    // but if that is still an issue for one reason or another...
    //
    // You can manually introduce a *continuous* increase of the inner(outer)_pcf_offset here
    // based on some depth-like parameter to hide the aliasing for distant cascades.
    const vec2 base_cascade_scale  = cascade_params[0].scale.xy;
    const vec2 inner_cascade_scale = cascade_params[inner_cascade_id].scale.xy;
    const vec2 inner_pcf_offset    = pcf_offset * (base_cascade_scale / inner_cascade_scale);

    float shadow_yield = dir_shadow_yield_pcf_sample(
        pcf_extent, pcf_order, inner_pcf_offset,
        texel_size, inner_cascade_id, normal_light_space, inner_frag_pos_ndc
    );


    if (dir_shadow.do_blend_cascades) {

        if (outer_cascade_blend > 0.0) {
            int outer_cascade_id = inner_cascade_id + 1;

            const mat4 outer_cascade_pv           = cascade_params[outer_cascade_id].projview;
            const vec4 outer_frag_pos_light_space = outer_cascade_pv * vec4(frag_pos, 1.0);
            const vec3 outer_frag_pos_ndc =
                0.5 * (1.0 + outer_frag_pos_light_space.xyz / outer_frag_pos_light_space.w);

            const vec2 outer_cascade_scale = cascade_params[outer_cascade_id].scale.xy;
            const vec2 outer_pcf_offset    = pcf_offset * (base_cascade_scale / outer_cascade_scale);

            const float outer_shadow_yield = dir_shadow_yield_pcf_sample(
                pcf_extent, pcf_order, outer_pcf_offset,
                texel_size, outer_cascade_id, normal_light_space, outer_frag_pos_ndc
            );

            // FIXME: There's a bit of self-shadowing in the blend region that can
            // be eliminated with a small extra bias. But it shouldn't be there
            // in the first place. Both blending and pcf_offset accentuate it. Investigate.
            shadow_yield = mix(shadow_yield, outer_shadow_yield, smoothstep(0.0, 1.0, outer_cascade_blend));
        }

    }


    return shadow_yield;
}




float dir_shadow_yield_pcf_sample(
    int pcf_extent, int pcf_order, vec2 pcf_offset_tx,
    vec2 texel_size, int cascade_id, vec3 normal_light_space, vec3 frag_pos_ndc)
{
    // In light space.
    const vec3  light_back_dir      = vec3(0.0,  0.0,  1.0);
    const float incidence_angle_cos = dot(light_back_dir, normal_light_space);


    const float mean_texel_size = (texel_size.x + texel_size.y) * 0.5;
    const float base_bias       = dir_shadow.base_bias_tx * mean_texel_size;

    // Do PCF sampling.

    float yield = 0.0;

    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {

            const vec2 patch_offset = vec2(x, y) * pcf_offset_tx * texel_size;

            // Here we calculate per-sample bias assuming planar geometry, where normals for each
            // PCF sample are assumed the same as the normal for the central fragment.
            //
            // The idea here is very similar to the "receiver plane depth bias" or other
            // "adaptive bias" techniques. The implementation here might look different
            // for one reason or another, but it's the same "select bias for each sample
            // to tightly fit the assumed planar geometry" thing. I just did this on my own
            // at first, and then figured that there's probably at least a dozen papers about
            // something similar to this. There is.
            //
            //
            // Each 2x2 PCF sample is a square "patch" with the shadow-test already interpolated
            // by the hardware. We sample these patches, instead of sampling each texel and doing
            // our own bilinear filtering of the shadow-testing. This allows us to easily adjust
            // the pcf_offset and control the effective filtering area in world-space.
            // For small PCF scales this looks good enough, for proper filtering textureGather()
            // can be used instead.
            //
            // The central patch has it's midpoint intersecting the geometry plane at exactly frag_depth.
            // For calculating the bias and sink (see below), we assume this intersection point to be the origin.
            //
            //
            // If the light and normal vectors are not collinear, then some patches will be sunken
            // below, partially intersecting, or floating above the geometry plane.
            //
            // If the patch is partially or fully sunken into the plane at one of the sides,
            // there exists a point (a patch vertex) where the depth of the sinking is maximal.
            //
            // The patch has to be "lifted" along the light direction such that there's no more sinking
            // anywhere on the patch. The distance to lift the patch by defines its bias.
            //
            // Floating patches are instead lowered towards the plane to "just touch" it
            // by applying negative bias.


            // The 2x2 patch for PCF is probably chosen from a quadrant of a texel, but not sure.
            // The multiplier of 1.5 accounts for the maximum quadrant dimensions of 0.5x0.5
            // plus the size of the adjacent texel in X and Y; this defines the dimensions of each patch.
            const vec2 ps = 1.5 * texel_size;

            // We take each of the vertices of the patch, and find their signed distance
            // from the plane by representing the plane by its normal.
            const vec3 patch_vertices[4] = {
                vec3(patch_offset, 0.0) + vec3(-ps.x,  ps.y, 0.0),
                vec3(patch_offset, 0.0) + vec3( ps.x,  ps.y, 0.0),
                vec3(patch_offset, 0.0) + vec3(-ps.x, -ps.y, 0.0),
                vec3(patch_offset, 0.0) + vec3( ps.x, -ps.y, 0.0)
            };

            // Since we calclulate sink in the space where the midpoint of the central patch
            // is the origin, the geometry plane is defined by the equation:
            //     Ax + By + Cz + D = 0,
            // where its normal vector is (A, B, C) and D is normally-projected frag_depth,
            // which in this space equals 0, since the midpoint of the central patch lies in
            // the geometry plane. The signed distance of a point r from such plane is just:
            //     S = Ax + By + Cz = dot(n, r)
            //
            // We calculate how much the patch vertices sink against the normal of the plane,
            // then we find the maximum sink. Note that the sink is calculated *against* the normal.
            // Positive sink means we need to do bias-correction by lifting the patch up -
            // above the surface, negative sink means we need to lower the patch down.
            const float normal_sink[4] = {
                dot(-normal_light_space, patch_vertices[0]),
                dot(-normal_light_space, patch_vertices[1]),
                dot(-normal_light_space, patch_vertices[2]),
                dot(-normal_light_space, patch_vertices[3])
            };

            const float max_normal_sink = max(
                max(normal_sink[0], normal_sink[1]),
                max(normal_sink[2], normal_sink[3])
            );

            // Now we just project the normal sink onto the light direction.
            // This can be simply done with the cosine of the incidence angle.
            const float max_sink = max_normal_sink / incidence_angle_cos;

            // We lift the patch by -max_sink plus additional base_bias.
            //
            // Some small base bias is still needed to account for two things:
            //   1. Interpolated normals;
            //   2. Normal-mapped surfaces.
            // Both of them distort real geometric normals which are needed for precise
            // geometric plane reconstruction, but are not usually present in the G-Buffer.
            //
            // Real geometric normals can be generated even for assets with interpolated
            // normals with the geometry shader and then later stored in a separate layer
            // of the G-Buffer at some performance and convinience cost. We don't do this here.
            //
            // Alternatively, geometric normals can be reconstructed from the depth buffer
            // using the depth gradient. However, that probably won't behave well
            // at the sharp edges of geometry. We don't do this here either.
            //
            // Base bias does not have to be large, you can get by with a bias of
            // only about a fraction of the shadow map texel size. You might
            // lose some very small scale detail, but you can compensate with AO or
            // contact shadows.
            const float sample_bias = -max_sink + base_bias;

            const float reference_depth = frag_pos_ndc.z - sample_bias;

            const vec4 lookup = vec4(
                frag_pos_ndc.xy + patch_offset, // Actual coordinates
                cascade_id,                     // Texture index
                reference_depth                 // Reference value for shadow-testing
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
