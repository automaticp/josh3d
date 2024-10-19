#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL
// #version 330 core


struct AmbientLight {
    vec3 color;
};


struct DirectionalLight {
    vec3 color;
    vec3 direction;
};


struct PointLight {
    vec3 color;
    vec3 position;
};


struct PointLightBounded {
    vec3  color;
    vec3  position;
    float radius;
};


float get_distance_attenuation(float distance_to_frag) {
    const float four_pi              = 4.0 * 3.141593;
    const float distance_attenuation = 1.0 / (four_pi * distance_to_frag * distance_to_frag);
    return distance_attenuation;
}


#endif
