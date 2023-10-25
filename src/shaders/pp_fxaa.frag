#version 430 core

in  vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D color;

uniform float gamma = 2.2;
uniform float absolute_contrast_threshold = 0.0312;
uniform float relative_contrast_threshold = 0.125;





float luma(vec3 rgb) {
    // const vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    const vec3 weights = vec3(0.299, 0.587, 0.114);
    return pow(dot(rgb, weights), 1.0 / gamma);
}


struct LumaInfo {
    float m, n, e, s, w;
    float ne, nw, se, sw;
    float hi, lo, contrast;
};


LumaInfo luminance_info_at_pixel(vec2 tex_uv, vec3 center_pixel_color) {
    LumaInfo l;

    l.m  = luma(center_pixel_color);
    l.n  = luma(textureLodOffset(color, tex_uv, 0, ivec2( 0,  1)).rgb);
    l.e  = luma(textureLodOffset(color, tex_uv, 0, ivec2( 1,  0)).rgb);
    l.s  = luma(textureLodOffset(color, tex_uv, 0, ivec2( 0, -1)).rgb);
    l.w  = luma(textureLodOffset(color, tex_uv, 0, ivec2(-1,  0)).rgb);
    l.ne = luma(textureLodOffset(color, tex_uv, 0, ivec2( 1,  1)).rgb);
    l.nw = luma(textureLodOffset(color, tex_uv, 0, ivec2(-1,  1)).rgb);
    l.se = luma(textureLodOffset(color, tex_uv, 0, ivec2( 1, -1)).rgb);
    l.sw = luma(textureLodOffset(color, tex_uv, 0, ivec2(-1, -1)).rgb);

    l.lo = min(l.m, min(min(l.n, l.s), min(l.e, l.w)));
    l.hi = max(l.m, max(max(l.n, l.s), max(l.e, l.w)));

    l.contrast = l.hi - l.lo;

    return l;
}


float pixel_blend_factor(LumaInfo l) {
    float weighted_sum = // of surrounding pixels
        2 * (l.n  + l.e  + l.s  + l.w) +
        1 * (l.ne + l.nw + l.se + l.sw);
    float avg_luma = weighted_sum / 12;
    float avg_jump = abs(avg_luma - l.m);

    float sharpness_factor = avg_jump / l.contrast;

    float blend_factor = smoothstep(0.0, 1.0, sharpness_factor);
    return blend_factor * blend_factor;
}




struct EdgeInfo {
    bool  is_horizontal;
    float tx_step; // sample offset to produce blending effect
};


float blend_dir_sign(float neg_luma, float mid_luma, float pos_luma) {
    float pos_grad = abs(pos_luma - mid_luma);
    float neg_grad = abs(neg_luma - mid_luma);
    return pos_grad > neg_grad ? 1.0 : -1.0;
}


EdgeInfo edge_info_around_pixel(LumaInfo l) {
    EdgeInfo e;

    float horizontal_contrast =
        2 * abs(l.n  + l.s  - 2 * l.m) +
        1 * abs(l.ne + l.se - 2 * l.e) +
        1 * abs(l.nw + l.sw - 2 * l.w);

    float vertical_contrast =
        2 * abs(l.e  + l.w  - 2 * l.m) +
        1 * abs(l.ne + l.nw - 2 * l.n) +
        1 * abs(l.se + l.sw - 2 * l.s);

    e.is_horizontal = horizontal_contrast >= vertical_contrast;

    vec2 tx_size = 1.0 / textureSize(color, 0);

    float axial_dir_sign =
        e.is_horizontal ?
            blend_dir_sign(l.s, l.m, l.n) :
            blend_dir_sign(l.w, l.m, l.e);

    e.tx_step = tx_size[uint(e.is_horizontal)] * axial_dir_sign;

    return e;
}


void main() {
    vec3 center_pixel_color = textureLod(color, tex_coords, 0).rgb;

    LumaInfo l = luminance_info_at_pixel(tex_coords, center_pixel_color);


    float contrast_threshold =
        max(absolute_contrast_threshold, l.hi * relative_contrast_threshold);

    if (l.contrast > contrast_threshold) {
        float blend_factor = pixel_blend_factor(l);
        EdgeInfo edge      = edge_info_around_pixel(l);

        vec2 sample_uv = tex_coords;
        sample_uv[uint(edge.is_horizontal)] += edge.tx_step * blend_factor;

        // Sampling in a branch is OK as long as you use textureLod().
        vec3 pixel_color = textureLod(color, sample_uv, 0).rgb;
        frag_color = vec4(pixel_color, 1.0);
    } else {
        frag_color = vec4(center_pixel_color, 1.0);
    }

    // TODO: No edge traversal yet.
}

