#version 330 core
#ifdef ENABLE_ALPHA_TESTING

in vec2 uv;

uniform struct Material
{
    sampler2D diffuse;
} material;

#endif // ENABLE_ALPHA_TESTING

void main()
{
#ifdef ENABLE_ALPHA_TESTING
    if (texture(material.diffuse, uv).a < 0.25) discard;
#endif // ENABLE_ALPHA_TESTING

    // gl_FragDepth = gl_FragCoord.z;
}
