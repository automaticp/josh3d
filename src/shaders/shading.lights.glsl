// #version 430 core
#ifndef SHADING_LIGHTS_GLSL
#define SHADING_LIGHTS_GLSL


/*
Basic spectral irradiance model with quadratic attenuation.
*/
vec3 point_light_irradiance(
    vec3  spectral_power,
    float light_to_frag_distance)
{
    const float four_pi = 4.0 * 3.141593;
    const float r       = light_to_frag_distance;
    const float attenuation = 1.0 / (four_pi * r * r);
    return spectral_power * attenuation;
}

/*
A [0, 1] factor that smoothly fades the point light near the edge of its volume.
*/
float point_light_volume_edge_fade(
    float volume_radius,
    float light_to_frag_distance,
    float start_radius_fraction) // Fraction of radius, where to start the fade.
{
    const float r  = light_to_frag_distance;
    const float r0 = start_radius_fraction * volume_radius;
    const float r1 = volume_radius;
    const float fade_factor = 1.0 - smoothstep(r0, r1, r);
    return fade_factor;
}


#endif
