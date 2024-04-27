// #version 330 core


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


float get_attenuation_factor(Attenuation att, float source_distance) {
    return 1.0 / (
        att.constant +
        att.linear * source_distance +
        att.quadratic * (source_distance * source_distance)
    );
}

