// #version 330 core


// Depth from 0 to z_far.
float get_view_space_depth(float screen_depth, float z_near, float z_far) {
    // Linearization back from non-linear depth encoding:
    //   screen_depth = (1/z - 1/n) / (1/f - 1/n)
    // Where z, n and f are: view-space z, z_near and z_far respectively.
    return (z_near * z_far) /
        (z_far - screen_depth * (z_far - z_near));
}


// Depth from 0 to 1.
float get_linear_0to1_depth(float screen_depth, float z_near, float z_far) {
    // screen_depth = (1/z - 1/n) / (1/f - 1/n)
    return z_near /
        (z_far - screen_depth * (z_far - z_near));
}


// ws  - world space,
// ts  - tangent space,
// vs  - view space,
// ps  - projection space (clip space before perspective division)
// ndc - normalized device coordinates (aka. clip space)
// nss - normalized screen space (ndc/2 + 1/2) in [0, 1] (vec3(uv, screen_z))


vec3 nss_to_ndc(vec3 nss) {
    return nss * 2.0 - 1.0;
}

vec3 ndc_to_nss(vec3 ndc) {
    return (ndc + 1.0) * 0.5;
}


vec4 ndc_to_vs(vec3 ndc, float z_near, float z_far, mat4 inv_proj) {
    // TODO: It might be pissible to extract z_near/far from the inv_proj.
    float screen_z = (ndc.z + 1.0) * 0.5;
    float clip_w   = get_view_space_depth(screen_z, z_near, z_far);
    vec4  ps       = vec4(ndc, 1.0) * clip_w;
    vec4  vs       = inv_proj * ps;
    return vs;
}


vec4 nss_to_vs(vec3 nss, float z_near, float z_far, mat4 inv_proj) {
    float screen_z = nss.z;
    float clip_w   = get_view_space_depth(screen_z, z_near, z_far);
    vec3  ndc      = nss_to_ndc(nss);
    vec4  ps       = vec4(ndc, 1.0) * clip_w;
    vec4  vs       = inv_proj * ps;
    return vs;
}


vec3 vs_to_ndc(vec4 vs, mat4 proj) {
    vec4 ps  = proj * vs;
    vec3 ndc = ps.xyz / ps.w;
    return ndc;
}


vec3 vs_to_nss(vec4 vs, mat4 proj) {
    vec3 ndc = vs_to_ndc(vs, proj);
    vec3 nss = ndc * 0.5 + 0.5;
    return nss;
}
