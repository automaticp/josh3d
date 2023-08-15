#version 330 core

in Interface {
    vec4 frag_pos;
#ifdef ENABLE_ALPHA_TESTING
    vec2 tex_coords;
#endif // ENABLE_ALPHA_TESTING
} in_;


#ifdef ENABLE_ALPHA_TESTING
uniform struct Material {
    sampler2D diffuse;
} material;
#endif // ENABLE_ALPHA_TESTING



void main() {

#ifdef ENABLE_ALPHA_TESTING
    if (texture(material.diffuse, in_.tex_coords).a < 0.5) discard;
#endif // ENABLE_ALPHA_TESTING

    // gl_FragDepth = gl_FragCoord.z;

}
