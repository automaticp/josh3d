#version 430 core

/*
Originally based on:
    https://catlikecoding.com/unity/tutorials/advanced-rendering/fxaa/

Other resources used:
    https://blog.simonrodriguez.fr/articles/2016/07/implementing_fxaa.html
    https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
*/

in vec2 uv;

uniform sampler2D color;

uniform uint  debug_mode = 0;
uniform uint  luma_mode  = 0;   // Selects the algorithm for computing luma.
uniform float gamma      = 2.2; // TODO: Pass inv_gamma already.
uniform float absolute_contrast_threshold = 0.0312; //
uniform float relative_contrast_threshold = 0.125;  //
uniform float pixel_blend_strength        = 1.0;    // [0, 1]
uniform float gradient_threshold_fraction = 0.25;   // Fraction of the initial gradient that terminates the edge scan. Lower
uniform uint  stride_table_idx            = 2;      // The table of strides for each successive step of the edge scan. Controls quality and number of steps.
uniform float guess_jump                  = 1.0;    // Additional "guess" uv jump applied when edge scan reached the end without finding the end.

out vec4 frag_color;


const float stride_table_0[] = { 1,  1,   1,   1,   1, 1, 1, 1, 1, 1 };
const float stride_table_1[] = { 1,  1.5, 2,   2,   2, 2, 2, 2, 2, 4 };
const float stride_table_2[] = { 1,  1,   1,   1.5, 2, 2, 2, 2, 4, 4 };
const float stride_table_3[] = { 1,  1.5, 2,   4 };

const uint luma_mode_dot_inv_gamma   = 0; // pow(dot(rgb, weights), 1 / gamma)
const uint luma_mode_dot_sqrt        = 1; // sqrt(dot(rgb, weights))
const uint luma_mode_green_inv_gamma = 2; // pow(rgb.g, 1 / gamma)
const uint luma_mode_green_sqrt      = 3; // sqrt(rgb.g)
const uint luma_mode_rg_inv_gamma    = 4; // pow(dot(rgb.rg, weights_rg), 1 / gamma)
const uint luma_mode_rg_sqrt         = 5; // sqrt(dot(rgb.rg, weights_rg))


/*
FIXME: I don't like the naming convention. Use l, c, r, t, b.

struct LumaInfo
{
    float lt, t, rt;
    float l,  c, r;
    float lb, b, rb;
    float hi, lo, contrast;
};
*/
struct LumaInfo
{
    float m, n, e, s, w;
    float ne, nw, se, sw;
    float hi, lo, contrast;
};

LumaInfo luminance_info_at_pixel(vec2 pixel_uv, vec3 center_pixel_color);

float get_pixel_blend_factor(LumaInfo l);


struct EdgeInfo
{
    uint  axis;            // Direction of the edge itself. X = 0, Y = 1.
    uint  blend_axis;      // Direction *across* the edge. X = 0, Y = 1. Always inverse of `axis.
    float step_uv;         // Sample offset in UV space used to produce blending. Should not exceed one pixel along blend_axis.
    float max_gradient;    // Max gradient along the blend direction.
    float downstream_luma; // Luma of the sample towards the blend direction.
};

EdgeInfo edge_info_around_pixel(LumaInfo l);

float get_edge_blend_factor(LumaInfo l, EdgeInfo e, vec2 uv);


void main()
{
    const vec3 center_pixel_color = textureLod(color, uv, 0).rgb;

    const LumaInfo l = luminance_info_at_pixel(uv, center_pixel_color);

    const float contrast_threshold =
        max(absolute_contrast_threshold, l.hi * relative_contrast_threshold);

    // Keep original color if below threshold.
    if (l.contrast < contrast_threshold)
    {
        frag_color = vec4(center_pixel_color, 1.0);
        return;
    }

    const float pixel_blend_factor = get_pixel_blend_factor(l);

    // Subpixel blend factor.
    if (debug_mode == 1)
    {
        frag_color = vec4(vec3(pixel_blend_factor), 1);
        return;
    }

    const EdgeInfo edge = edge_info_around_pixel(l);

    // Horizontal/vertical edge gradients.
    if (debug_mode == 2 || debug_mode == 3)
    {
        const float px_size_uv = 1.0 / textureSize(color, 0)[edge.blend_axis];
        const float step_px = edge.step_uv / px_size_uv;
        // Green - Positive, Red - Negative.
        const vec3  channel_mask = step_px > 0 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        if (edge.axis == 0 && debug_mode == 2) // Horizontal.
        {
            frag_color = vec4(abs(step_px) * channel_mask, 1);
        }
        else if (edge.axis == 1 && debug_mode == 3) // Vertical
        {
            frag_color = vec4(abs(step_px) * channel_mask, 1);
        }
        else
        {
            frag_color = vec4(1);
        }
        return;
    }

    // Edge max gradient.
    if (debug_mode == 4)
    {
        frag_color = vec4(vec3(edge.max_gradient), 1);
        return;
    }

    const float edge_blend_factor = get_edge_blend_factor(l, edge, uv);

    // Edge blend factor.
    if (debug_mode == 5)
    {
        frag_color = vec4(vec3(edge_blend_factor), 1);
        return;
    }

    // Edge-only blend factor. No 0.
    if (debug_mode == 6)
    {
        if (edge_blend_factor != 0)
            frag_color = vec4(vec3(edge_blend_factor), 1);
        else
            frag_color = vec4(center_pixel_color, 1);
        return;
    }

    const float blend_factor = max(
        pixel_blend_factor * pixel_blend_strength,
        edge_blend_factor
    );

    // Total blend factor.
    if (debug_mode == 7)
    {
        frag_color = vec4(vec3(blend_factor, 0, 0), 1);
        return;
    }

    vec2 sample_uv = uv;
    sample_uv[edge.blend_axis] += edge.step_uv * blend_factor;

    // NOTE: Sampling in a branch is OK as long as you use textureLod().
    const vec3 pixel_color = textureLod(color, sample_uv, 0).rgb;
    frag_color = vec4(pixel_color, 1.0);
}

float luma_of(vec3 rgb)
{
    // const vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    const vec3 weights_rgb = vec3(0.299, 0.587, 0.114);
    const vec2 weights_rg  = vec2(0.337, 0.663);
    switch (luma_mode)
    {
        case luma_mode_dot_inv_gamma:
            return pow(dot(rgb, weights_rgb), 1 / gamma);
        case luma_mode_dot_sqrt:
            return sqrt(dot(rgb, weights_rgb));
        case luma_mode_green_inv_gamma:
            return pow(rgb.g, 1 / gamma);
        case luma_mode_green_sqrt:
            return sqrt(rgb.g);
        case luma_mode_rg_inv_gamma:
            return pow(dot(rgb.rg, weights_rg), 1 / gamma);
        case luma_mode_rg_sqrt:
            return sqrt(dot(rgb.rg, weights_rg));
    }
    return 0;
}

LumaInfo luminance_info_at_pixel(vec2 pixel_uv, vec3 center_pixel_color)
{
    LumaInfo l;

    l.m  = luma_of(center_pixel_color);
    l.n  = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2( 0,  1)).rgb);
    l.e  = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2( 1,  0)).rgb);
    l.s  = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2( 0, -1)).rgb);
    l.w  = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2(-1,  0)).rgb);
    l.ne = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2( 1,  1)).rgb);
    l.nw = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2(-1,  1)).rgb);
    l.se = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2( 1, -1)).rgb);
    l.sw = luma_of(textureLodOffset(color, pixel_uv, 0, ivec2(-1, -1)).rgb);

    l.lo = min(l.m, min(min(l.n, l.s), min(l.e, l.w)));
    l.hi = max(l.m, max(max(l.n, l.s), max(l.e, l.w)));

    l.contrast = l.hi - l.lo;

    return l;
}

float get_pixel_blend_factor(LumaInfo l)
{
    const float weighted_sum = // of surrounding pixels.
        2 * (l.n  + l.e  + l.s  + l.w) +
        1 * (l.ne + l.nw + l.se + l.sw);

    const float avg_luma = weighted_sum / 12;
    const float avg_jump = abs(avg_luma - l.m);

    const float sharpness_factor = avg_jump / l.contrast;

    const float blend_factor = smoothstep(0.0, 1.0, sharpness_factor);
    return blend_factor * blend_factor;
}

EdgeInfo edge_info_around_pixel(LumaInfo l)
{
    EdgeInfo e;

    /*
                ^ positive y direction

        --------- horizontal edge (axis = 0)

                v negative y direction
    */

    /*
        > positive x direction
        |
        | vertical edge (axis = 1)
        |
        < negative x direction
    */

    const float y_contrast =
        2 * abs(l.n  + l.s  - 2 * l.m) +
        1 * abs(l.ne + l.se - 2 * l.e) +
        1 * abs(l.nw + l.sw - 2 * l.w);

    const float x_contrast =
        2 * abs(l.e  + l.w  - 2 * l.m) +
        1 * abs(l.ne + l.nw - 2 * l.n) +
        1 * abs(l.se + l.sw - 2 * l.s);

    e.blend_axis = uint(y_contrast >= x_contrast);
    e.axis       = uint(!bool(e.blend_axis));

    const float neg_luma = e.blend_axis == 0 ? l.w : l.s;
    const float mid_luma = l.m;
    const float pos_luma = e.blend_axis == 0 ? l.e : l.n;

    const float pos_grad = abs(pos_luma - mid_luma);
    const float neg_grad = abs(neg_luma - mid_luma);

    e.max_gradient    = max(pos_grad, neg_grad);
    e.downstream_luma = pos_grad > neg_grad ? pos_luma : neg_luma;

    const vec2 px_size_uv = 1.0 / textureSize(color, 0);

    e.step_uv = px_size_uv[e.blend_axis] * sign(pos_grad - neg_grad);

    return e;
}

float luma_at(vec2 uv)
{
    return luma_of(textureLod(color, uv, 0).rgb);
}

struct EdgeScan
{
    float dluma;    // Luminance delta at the end point.
    vec2  uv;       // UV position at the end point.
    vec2  duv;      // Full UV distance covered by the scan.
};

int get_stride_table_length()
{
    switch (stride_table_idx)
    {
        case 0: return stride_table_0.length();
        case 1: return stride_table_1.length();
        case 2: return stride_table_2.length();
        case 3: return stride_table_3.length();
    }
    return 0;
}

float get_stride(int i)
{
    switch (stride_table_idx)
    {
        case 0: return stride_table_0[i];
        case 1: return stride_table_1[i];
        case 2: return stride_table_2[i];
        case 3: return stride_table_3[i];
    }
    return 0.0;
}

void scan_edge(
    out EdgeScan lscan,
    out EdgeScan rscan,
    vec2         start_uv,
    vec2         step_duv,
    float        luma_at_start,
    float        gradient_threshold)
{
    const int max_steps = get_stride_table_length();

    bool lend = false;
    bool rend = false;

    lscan.uv = start_uv;
    rscan.uv = start_uv;

#if 0

    for (int i = 0; i < max_steps; ++i)
    {
        const float stride = get_stride(i);

        if (!lend)
        {
            lscan.uv   -= stride * step_duv;
            lscan.dluma = luma_at(lscan.uv) - luma_at_start;
        }

        if (!rend)
        {
            rscan.uv   += stride * step_duv;
            rscan.dluma = luma_at(rscan.uv) - luma_at_start;
        }

        lend = lend || abs(lscan.dluma) >= gradient_threshold;
        rend = rend || abs(rscan.dluma) >= gradient_threshold;

        if (lend && rend) break;
    }

#else // This is just a tiny bit faster.

    for (int i = 0; i < max_steps; ++i)
    {
        const float stride = get_stride(i);
        lscan.uv   -= stride * step_duv;
        lscan.dluma = luma_at(lscan.uv) - luma_at_start;
        lend        = lend || abs(lscan.dluma) >= gradient_threshold;
        if (lend) break;
    }

    for (int i = 0; i < max_steps; ++i)
    {
        const float stride = get_stride(i);
        rscan.uv   -= stride * step_duv;
        rscan.dluma = luma_at(rscan.uv) - luma_at_start;
        rend        = rend || abs(rscan.dluma) >= gradient_threshold;
        if (rend) break;
    }

#endif

    if (!lend) lscan.uv -= guess_jump * step_duv;
    if (!rend) rscan.uv += guess_jump * step_duv;

    lscan.duv = lscan.uv - start_uv;
    rscan.duv = rscan.uv - start_uv;
}

float get_edge_blend_factor(LumaInfo l, EdgeInfo e, vec2 start_uv)
{
    start_uv[e.blend_axis] += 0.5 * e.step_uv;

    // This is getting really tedious. Can we just go to edge space alredy?
    const vec2 px_size_uv = 1.0 / textureSize(color, 0);

    // UV abs delta taken each step.
    vec2 duv;
    duv[e.blend_axis] = 0;
    duv[e.axis]       = px_size_uv[e.axis];

    const int max_steps = 10;

    // This is exactly at the midpoint between 2 pixels, in the gradient direction.
    const float edge_luma = 0.5 * (l.m + e.downstream_luma);
    const float gradient_threshold = gradient_threshold_fraction * e.max_gradient;

    // Perform left and light scans in edge space.
    EdgeScan scans[2];
    scan_edge(scans[0], scans[1], start_uv, duv, edge_luma, gradient_threshold);

    // Distance magnitudes to left and right endpoints.
    const float distances[2] = {
        abs(scans[0].duv[e.axis]),
        abs(scans[1].duv[e.axis]),
    };

    // Shortest scan index (0 - left, 1 - right);
    const uint ssidx = distances[0] < distances[1] ? 0 : 1;

    // Distance to the closest end.
    const float end_distance = distances[ssidx];

    // True if the end is darker than start, false if brighter.
    const bool is_end_darkening = (scans[ssidx].dluma < 0);

    // True if the target pixel is darker than start, false if brighter.
    const bool is_start_darkening = (l.m >= edge_luma);

    // Blend towards direction of lowered contrast (relax).
    // If the closest end increases the contrast instead, bail.

    const bool is_lowered_contrast =
        is_start_darkening == is_end_darkening;

    if (!is_lowered_contrast)
        return 0;

    const float blend_factor =
        0.5 - end_distance / (distances[0] + distances[1]);

    return blend_factor;
}
