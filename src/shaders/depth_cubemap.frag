#version 330 core

in vec4 frag_pos;
in vec3 light_pos;


#ifdef ENABLE_ALPHA_TESTING

in vec2 tex_coords;

uniform struct Material {
    sampler2D diffuse;
} material;

#endif // ENABLE_ALPHA_TESTING


uniform float z_far;
void main() {

#ifdef ENABLE_ALPHA_TESTING
    if (texture(material.diffuse, tex_coords).a < 0.5) discard;
#endif // ENABLE_ALPHA_TESTING

    // TODO: Why is this needed again?
    // This breaks early z-rejection.

    float light_to_frag_distance =
        length(frag_pos.xyz - light_pos) / z_far;

    gl_FragDepth = light_to_frag_distance;
}
