// #version 430 core
#ifndef SHADING_CSM_GLSL
#define SHADING_CSM_GLSL


struct CascadeView {
    mat4 projview;
    vec3 scale;
};


/*
This dumb mumbo jumbo is here because I can't pass
dynamically sized arrays to functions.

Passing fixed sized arrays instead is incredibly slow.

GLSL is truly a kind of a shading language.
*/
#ifndef CASCADE_VIEWS_SSBO_BINDING
#define CASCADE_VIEWS_SSBO_BINDING 0
#endif


layout (std430, binding = CASCADE_VIEWS_SSBO_BINDING) restrict readonly
buffer _CascadeViewsBlock {
    CascadeView cascade_views[];
} _cvb;


struct CascadeShadowParams {
    float base_bias_tx;
    float blend_size_best_tx;
    int   pcf_extent;
    float pcf_offset_inner_tx;
};


float directional_light_shadow_obscurance(
    sampler2DArrayShadow cascade_maps,
//  CascadeView          cascade_views[],
    CascadeShadowParams  params,
    vec3                 frag_pos_ws,
    vec3                 normal_ws);




/*
Implementation of CSM shadows is below.

Some notable terminology about particular cascades:

- "Inner" cascade refers to the innermost cascade of the chain, the one with index 0.
  It is used as a reference cascade for some parameters that are reliant on scaling.

- "Best" cascade is the best resolution cascade available for the fragment.
  This is the primary cascade in selection, the one we are 100% going to sample.

- "Next" cascade is the next best resolution cascade after the "best" one.
  This one may be sampled in addition to "best" cascade, in case of blending.


I use abbreviations to refer to specific "spaces":

`ws`:
    World space.

`ls` or `cs`:
    Clip-space of the shadow "camera" (aka. light-space).

`ndc`:
    Normalized device coordinates. Clip-space after perspective division with [-1, 1] range on xyz.
    For orthographic projection W is 1, and perspective division is identity, so NDC == clip-space.

`nss`:
    Normalized screen space. Basically `(ndc + 1) / 2`, which ends up in [0, 1] range on xyz.
    "Screen" in this case refers to the implied screen of the shadow camera.
    The XY components of `nss` can be used to sample shadowmap textures. The Z component -
    to compare depths using the `shadow` sampler.

`uv`:
    The XY components of the `nss`.

`tx`:
    Texel units/texel space of the shadowmap. Ratio to world units differs per cascade.
    1 in texel space covers one shadowmap texel (ikr).


The abbreviations are convenient to perform quick sanity-checks on each calculation.

*/


struct _CascadeSelection {
    int   best_cascade_id;
    float next_cascade_blend_factor;
};


_CascadeSelection _select_best_cascade_with_blending(
//  CascadeView cascade_views[],
    vec3        frag_pos_ws,
    vec3        pcf_padding_inner_ndc,
    vec3        blend_padding_ndc);


int _select_best_cascade(
//  CascadeView cascade_views[],
    vec3        frag_pos_ws,
    vec3        pcf_padding_inner_ndc);


float _receiver_plane_bias(
    vec2  patch_offset_uv,
    vec2  texel_size_uv,
    vec3  normal_nss);


float _directional_shadow_pcf_sample(
    sampler2DArrayShadow cascade_maps,
    int   cascade_id,
    float base_bias_tx,
    int   pcf_extent,
    int   pcf_order,
    vec2  pcf_offset_uv,
    vec2  texel_size_uv,
    vec3  normal_nss,
    vec3  frag_pos_nss);





float directional_light_shadow_obscurance(
    sampler2DArrayShadow cascade_maps,
//  CascadeView          cascade_views[],
    CascadeShadowParams  params,
    vec3                 frag_pos_ws,
    vec3                 normal_ws)
{
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
    //     texel_size_world_space = texel_size_uv * cascade_params[cascade_id].scale.xy;
    //
    // TODO: It might make sense to derive the offset from some parameter that defines
    // the effective PCF area instead (like pcf_width or something), so that the area
    // is preserved between different kernel sizes and shadowmap resolutions.
    // I'm just too lazy right now.

    const int   pcf_extent          = params.pcf_extent;
    const int   pcf_order           = 1 + 2 * pcf_extent;
    const vec2  pcf_offset_inner_tx = vec2(params.pcf_offset_inner_tx);

    const vec2  texel_size_uv       = 1.0 / textureSize(cascade_maps, 0).xy;
    const vec2  pcf_offset_inner_uv = texel_size_uv * pcf_offset_inner_tx;

    // We pad each cascade map by the (extent * offset) of the PCF kernel
    // in order to eliminate the gaps between cascades, that normally
    // arise due to sampling over the edge of the cascade map.
    //
    // We also add an extra texel_size padding to account for the fact
    // that the shadow sampler already provides us with the 2x2 PCF
    // on a non-configurable texel_scale.
    //
    // The coefficient 2 is here because we pad in NDC which is [-1, 1]
    // but sample in NSS(UV) which is [0, 1], so the padding has to be twice as wide.

    // FIXME: Uh, the above sounds like a complete opposite of what it should be.

    // PCF padding will be scaled to each cascade such that it's constant *in world-space*.
    const vec2 pcf_padding_inner     = 2.0 * (pcf_extent * pcf_offset_inner_uv + texel_size_uv);
    const vec3 pcf_padding_inner_ndc = vec3(pcf_padding_inner, 0.0);

    // Use the value of blend size to determine if blending is needed.
    const bool do_blend = (params.blend_size_best_tx != 0.0);

    // For blending we use the terms like "best" and "next" cascades:
    //   - Best cascade is just the best resolution cascade available for the fragment;
    //   - Next cascade is the next best resolution cascade after the best one.
    int   best_cascade_id;
    float next_cascade_blend = 0.0;

    if (do_blend) {
        // Blend padding does not scale with the cascades, and always remains
        // as N texels of the "best" cascade for a given selection.
        const float blend_size_best_tx     = params.blend_size_best_tx;
        const vec2  blend_padding_best     = blend_size_best_tx * texel_size_uv * 0.5;
        const vec3  blend_padding_best_ndc = vec3(blend_padding_best, 0.0);

        const _CascadeSelection selection =
            _select_best_cascade_with_blending(
            //  cascade_views,
                frag_pos_ws,
                pcf_padding_inner_ndc,
                blend_padding_best_ndc
            );

        best_cascade_id    = selection.best_cascade_id;
        next_cascade_blend = selection.next_cascade_blend_factor;

    } else {
        best_cascade_id =
            _select_best_cascade(
            //  cascade_views,
                frag_pos_ws,
                pcf_padding_inner_ndc
            );
    }

    // Everything that follows with biasing and PCF sampling
    // is easier done in NSS, so we go there.

    // Get the position and normal in clip-space == light-space == NDC (for ortho).
    const mat4 best_cascade_projview = _cvb.cascade_views[best_cascade_id].projview;
    const vec3 frag_pos_best_ndc     = vec3(best_cascade_projview * vec4(frag_pos_ws, 1.0));
    const vec3 frag_pos_best_nss     = 0.5 * (1.0 + frag_pos_best_ndc);

    // FIXME: Still need to fix the bug where the cascades cut-off early.
    if (frag_pos_best_nss.z > 1.0) { return 0.0; }

    // All cascades are parallel, so any cascade will work for plane normals.
    const mat3 cascade_normal_projview = mat3(transpose(inverse(best_cascade_projview)));
    const vec3 normal_ndc              = normalize(cascade_normal_projview * normal_ws);
    // Exactly the same, translation and uniform scaling does not change normals.
    const vec3 normal_nss              = normal_ndc;

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
    const vec2 inner_cascade_scale = _cvb.cascade_views[0].scale.xy;
    const vec2 best_cascade_scale  = _cvb.cascade_views[best_cascade_id].scale.xy;
    const vec2 pcf_offset_best_uv  = pcf_offset_inner_uv * (inner_cascade_scale / best_cascade_scale);

    const float obscurance_best =
        _directional_shadow_pcf_sample(
            cascade_maps,
            best_cascade_id,
            params.base_bias_tx,
            pcf_extent,
            pcf_order,
            pcf_offset_best_uv,
            texel_size_uv,
            normal_nss,
            frag_pos_best_nss
        );

    if (do_blend) {

        if (next_cascade_blend > 0.0) {
            int next_cascade_id = best_cascade_id + 1;

            const mat4 next_cascade_projview = _cvb.cascade_views[next_cascade_id].projview;
            const vec3 frag_pos_next_ndc     = vec3(next_cascade_projview * vec4(frag_pos_ws, 1.0));
            const vec3 frag_pos_next_nss     = 0.5 * (1.0 + frag_pos_next_ndc);

            const vec2 next_cascade_scale = _cvb.cascade_views[next_cascade_id].scale.xy;
            const vec2 pcf_offset_next_uv = pcf_offset_inner_uv * (inner_cascade_scale / next_cascade_scale);

            const float obscurance_next =
                _directional_shadow_pcf_sample(
                    cascade_maps,
                    next_cascade_id,
                    params.base_bias_tx,
                    pcf_extent,
                    pcf_order,
                    pcf_offset_next_uv,
                    texel_size_uv,
                    normal_nss,
                    frag_pos_next_nss
                );

            // FIXME: There's a bit of self-shadowing in the blend region that can
            // be eliminated with a small extra bias. But it shouldn't be there
            // in the first place. Both blending and pcf_offset accentuate it. Investigate.
            return mix(obscurance_best, obscurance_next, smoothstep(0.0, 1.0, next_cascade_blend));
        }

    }

    return obscurance_best;
}




bool _is_fragment_in_padded_box(
    vec3 frag_pos_ndc,
    vec3 padding_ndc)
{
    return all(lessThan(abs(frag_pos_ndc), vec3(1.0) - padding_ndc));
}


_CascadeSelection _select_best_cascade_with_blending(
//  CascadeView cascade_views[],
    vec3        frag_pos_ws,
    vec3        pcf_padding_inner_ndc,
    vec3        blend_padding_ndc)
{
    const int num_cascades    = _cvb.cascade_views.length();
    const int last_cascade_id = num_cascades - 1;

    const vec3 inner_cascade_scale = _cvb.cascade_views[0].scale;

    // We assume that cascades are stored in order from smallest
    // to largest, so the first match is the smallest projection,
    // that is highest resolution match.
    for (int i = 0; i < last_cascade_id; ++i) {

        const CascadeView cascade = _cvb.cascade_views[i];

        const vec4 frag_pos_cs  = cascade.projview * vec4(frag_pos_ws, 1.0);
        const vec3 frag_pos_ndc = vec3(frag_pos_cs); // Abuse orthographic projection.

        const vec3 best_cascade_scale = cascade.scale;

        // We scale the PCF padding of the "best" cascade according to `scale` factor.
        //
        // TODO: Shouldn't we also check the scale of the next-best cascade?
        // For overlap. Is that why it's buggy at steep angles?
        //
        // But we don't scale the Blend padding, it's fixed in texel units.
        const vec3 pcf_padding_best_ndc = pcf_padding_inner_ndc * (inner_cascade_scale / best_cascade_scale);
        const vec3 total_padding_ndc    = pcf_padding_best_ndc + blend_padding_ndc;

        if (_is_fragment_in_padded_box(frag_pos_ndc, pcf_padding_best_ndc)) {
            if (_is_fragment_in_padded_box(frag_pos_ndc, total_padding_ndc)) {

                return _CascadeSelection(i, 0.0);

            } else /* Welcome to THE BLEND ZONE!!! */ {

                const vec3 blend_start_ndc = vec3(1.0) - total_padding_ndc;

                const vec3 next_blend_factors =
                    (abs(frag_pos_ndc) - blend_start_ndc) / blend_padding_ndc;

                // TODO: Maybe return dimensional blend factors?
                const float next_blend_factor =
                    max(next_blend_factors.x, next_blend_factors.y);

                return _CascadeSelection(i, next_blend_factor);
            }
        }
    }

    // If we hit the last cascade or there's only one cascade total,
    // then select it, no padding or blending.
    return _CascadeSelection(last_cascade_id, 0.0);
}




int _select_best_cascade(
//  CascadeView cascade_views[],
    vec3        frag_pos_ws,
    vec3        pcf_padding_inner_ndc)
{
    const int num_cascades    = _cvb.cascade_views.length();
    const int last_cascade_id = num_cascades - 1;

    const vec3 inner_cascade_scale = _cvb.cascade_views[0].scale;

    // We assume that cascades are stored in order from smallest
    // to largest, so the first match is the smallest projection,
    // that is highest resolution match.
    for (int i = 0; i < last_cascade_id; ++i) {

        const CascadeView cascade = _cvb.cascade_views[i];

        const vec4 frag_pos_cs  = cascade.projview * vec4(frag_pos_ws, 1.0);
        const vec3 frag_pos_ndc = vec3(frag_pos_cs);

        const vec3 best_cascade_scale   = cascade.scale;
        const vec3 pcf_padding_best_ndc = pcf_padding_inner_ndc * (inner_cascade_scale / best_cascade_scale);

        if (_is_fragment_in_padded_box(frag_pos_ndc, pcf_padding_best_ndc)) {
            return i;
        }

    }

    // If we hit the last cascade or there's only one cascade total,
    // then select it, no padding.
    return last_cascade_id;
}




float _receiver_plane_bias(
    vec2  patch_offset_uv,
    vec2  texel_size_uv,
    vec3  normal_nss)
{
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
    const vec2 ps = 1.5 * texel_size_uv;

    // We take each of the vertices of the patch, and find their signed distance
    // from the plane by representing the plane by its normal.
    const vec3 patch_vertices[4] = {
        vec3(patch_offset_uv, 0.0) + vec3(-ps.x,  ps.y, 0.0),
        vec3(patch_offset_uv, 0.0) + vec3( ps.x,  ps.y, 0.0),
        vec3(patch_offset_uv, 0.0) + vec3(-ps.x, -ps.y, 0.0),
        vec3(patch_offset_uv, 0.0) + vec3( ps.x, -ps.y, 0.0)
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
        dot(-normal_nss, patch_vertices[0]),
        dot(-normal_nss, patch_vertices[1]),
        dot(-normal_nss, patch_vertices[2]),
        dot(-normal_nss, patch_vertices[3])
    };

    const float max_normal_sink = max(
        max(normal_sink[0], normal_sink[1]),
        max(normal_sink[2], normal_sink[3])
    );

    // Now we just project the normal sink onto the light direction.
    // This can be simply done with the cosine of the incidence angle.
    const vec3  light_back_nss      = vec3(0.0,  0.0,  1.0);
    const float incidence_angle_cos = dot(light_back_nss, normal_nss);

    const float max_sink        = max_normal_sink / incidence_angle_cos;
    const float sample_bias_nss = -max_sink;

    return sample_bias_nss;
}


float _directional_shadow_pcf_sample(
    sampler2DArrayShadow cascade_maps,
    int   cascade_id,
    float base_bias_tx,
    int   pcf_extent,
    int   pcf_order,
    vec2  pcf_offset_uv,
    vec2  texel_size_uv,
    vec3  normal_nss,
    vec3  frag_pos_nss)
{
    // TODO: It's a square texture, chill.
    const float max_texel_extent = max(texel_size_uv.x, texel_size_uv.y);
    const float base_bias_nss    = base_bias_tx * max_texel_extent;

    float obscurance = 0.0;

    for (int x = -pcf_extent; x <= pcf_extent; ++x) {
        for (int y = -pcf_extent; y <= pcf_extent; ++y) {

            const vec2 patch_offset_uv = vec2(x, y) * pcf_offset_uv;

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
            // lose some very small scale detail, but you can maybe compensate with
            // contact shadows.
            const float bias_nss        = base_bias_nss + _receiver_plane_bias(patch_offset_uv, texel_size_uv, normal_nss);
            const float reference_z_nss = frag_pos_nss.z - bias_nss;

            const vec4 lookup = vec4(
                frag_pos_nss.xy + patch_offset_uv, // Actual coordinates;
                cascade_id,                        // Texture index;
                reference_z_nss                    // Reference value for shadow-testing.
            );

            const float lit = texture(cascade_maps, lookup).r;

            // Shadow sampler returns "how much of the fragment is lit"
            // from GL_LESS comparison function. We need "how much of
            // the fragment is in shadow".
            obscurance += (1.0 - lit);
        }
    }

    return obscurance / (pcf_order * pcf_order);
}




#endif
