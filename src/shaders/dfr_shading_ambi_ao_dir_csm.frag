#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "gbuffer.glsl"


in vec2  tex_coords;
out vec4 frag_color;




struct DirectionalShadow {
    sampler2DArrayShadow cascades;
    float base_bias_tx;
    bool  do_blend_cascades;
    float blend_size_inner_tx;
    bool  do_cast;
    int   pcf_extent;
    float pcf_offset;
};


struct CascadeParams {
    mat4  projview;
    vec3  scale;
    float z_split;
};


struct AmbientOcclusion {
    sampler2D tex_occlusion;
    bool      use;
    float     power;
};




uniform GBuffer gbuffer;

uniform AmbientLight      ambi_light;
uniform AmbientOcclusion  ambi_occlusion;
uniform DirectionalLight  dir_light;
uniform DirectionalShadow dir_shadow;

layout (std430, binding = 0) restrict readonly
buffer CascadeParamsBlock {
    CascadeParams cascade_params[];
};

uniform vec3 cam_pos;




float dir_light_shadow_obscurance(
    DirectionalShadow dir_shadow,
 /* CascadeParams     cascade_params[], */
    vec3              frag_pos,
    vec3              normal);


void add_dir_light_illumination(
    inout vec3        color,
    DirectionalLight  dir_light,
    float             obscurance,
    vec3              normal_dir,
    vec3              view_dir,
    vec3              albedo,
    float             specular);


float ambi_light_occlusion_obscurance(
    AmbientOcclusion ambi_occlusion);


void add_ambi_light_illumination(
    inout vec3   color,
    AmbientLight ambi_light,
    float        obscurance,
    vec3         albedo);




void main() {
    GSample gsample = unpack_gbuffer(gbuffer, tex_coords);
    if (!gsample.drawn) discard;

    const vec3 normal_dir = normalize(gsample.normal);
    const vec3 view_dir   = normalize(cam_pos - gsample.position);


    vec3 result_color = vec3(0.0);

    // Directional Light.
    {
        const float obscurance =
            dir_light_shadow_obscurance(
                dir_shadow,
                gsample.position,
                gsample.normal
            );

        add_dir_light_illumination(
            result_color,
            dir_light,
            obscurance,
            normal_dir,
            view_dir,
            gsample.albedo,
            gsample.specular
        );
    }

    // Ambient Light.
    {
        const float obscurance =
            ambi_light_occlusion_obscurance(
                ambi_occlusion
            );

        add_ambi_light_illumination(
            result_color,
            ambi_light,
            obscurance,
            gsample.albedo
        );
    }

    frag_color = vec4(result_color, 1.0);
}




void add_dir_light_illumination(
    inout vec3        color,
    DirectionalLight  dir_light,
    float             obscurance,
    vec3              normal_dir,
    vec3              view_dir,
    vec3              albedo,
    float             specular)
{
    const vec3 light_dir      = normalize(dir_light.direction);
    const vec3 reflection_dir = reflect(light_dir, normal_dir);

    const float diffuse_alignment  = max(dot(normal_dir, -light_dir),     0.0);
    const float specular_alignment = max(dot(view_dir,   reflection_dir), 0.0);

    const float diffuse_strength  = diffuse_alignment;
    const float specular_strength = pow(specular_alignment, 128.0);
    // FIXME: Hardcoded shininess because it is not stored in the GBuffer.

    color += dir_light.color * diffuse_strength  * albedo   * (1.0 - obscurance);
    color += dir_light.color * specular_strength * specular * (1.0 - obscurance);
}


void add_ambi_light_illumination(
    inout vec3   color,
    AmbientLight ambi_light,
    float        obscurance,
    vec3         albedo)
{
    color += ambi_light.color * albedo * (1.0 - obscurance);
}


float ambi_light_occlusion_obscurance(
    AmbientOcclusion ambi_occlusion)
{
    if (!ambi_occlusion.use) return 0.0;

    const float occlusion = textureLod(ambi_occlusion.tex_occlusion, tex_coords, 0).r;
    const float obscurance = 1.0 - pow(1.0 - occlusion, ambi_occlusion.power);

    return obscurance;
}




// Unhinged Shadow Mapping below, you've been warned.







bool is_fragment_in_clip(vec3 frag_pos_clip_space, vec3 clip_border_padding) {
    return all(lessThan(abs(frag_pos_clip_space), vec3(1.0) - clip_border_padding));
}


struct CascadeSelection {
    int   inner_cascade_id;
    float outer_cascade_blend_factor;
};


CascadeSelection select_best_cascade_blend(
    vec3 frag_pos_world_space,
    vec3 clip_discard_padding,
    vec3 clip_blend_padding)
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
    vec3 frag_pos_world_space,
    vec3 clip_discard_padding)
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




float dir_shadow_obscurance_pcf_sample(
    sampler2DArrayShadow cascades,
    int  cascade_id,
    vec3 frag_pos_ndc,
    int  pcf_extent,
    int  pcf_order,
    vec2 pcf_offset_tx,
    vec2 texel_size,
    vec3 normal_light_space);




float dir_light_shadow_obscurance(
    DirectionalShadow dir_shadow,
 /* CascadeParams     cascade_params[], */
    vec3              frag_pos,
    vec3              normal)
{
    if (!dir_shadow.do_cast) return 0.0;

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
    // but sample in NSS which is [0, 1], so the padding has to be twice as wide.
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

    // TODO: This is not NDC, but NSS.
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

    float obscurance =
        dir_shadow_obscurance_pcf_sample(
            dir_shadow.cascades,
            inner_cascade_id,
            inner_frag_pos_ndc,
            pcf_extent,
            pcf_order,
            inner_pcf_offset,
            texel_size,
            normal_light_space
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

            const float outer_obscurance =
                dir_shadow_obscurance_pcf_sample(
                    dir_shadow.cascades,
                    outer_cascade_id,
                    outer_frag_pos_ndc,
                    pcf_extent,
                    pcf_order,
                    outer_pcf_offset,
                    texel_size,
                    normal_light_space
                );

            // FIXME: There's a bit of self-shadowing in the blend region that can
            // be eliminated with a small extra bias. But it shouldn't be there
            // in the first place. Both blending and pcf_offset accentuate it. Investigate.
            obscurance = mix(obscurance, outer_obscurance, smoothstep(0.0, 1.0, outer_cascade_blend));
        }

    }


    return obscurance;
}




float dir_shadow_obscurance_pcf_sample(
    sampler2DArrayShadow cascades,
    int  cascade_id,
    vec3 frag_pos_ndc,
    int  pcf_extent,
    int  pcf_order,
    vec2 pcf_offset_tx,
    vec2 texel_size,
    vec3 normal_light_space)
{
    // In light space.
    const vec3  light_back_dir      = vec3(0.0, 0.0, 1.0);
    const float incidence_angle_cos = dot(light_back_dir, normal_light_space);


    const float mean_texel_size = (texel_size.x + texel_size.y) * 0.5;
    const float base_bias       = dir_shadow.base_bias_tx * mean_texel_size;

    // Do PCF sampling.

    float obscurance = 0.0;

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
            obscurance += (1.0 - pcf_lit);
        }
    }

    // Normalize and return.
    return obscurance / (pcf_order * pcf_order);

}
