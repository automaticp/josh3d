#version 330 core
#ifdef ENABLE_ALPHA_TESTING

in Interface
{
    vec2 uv;
} in_;

uniform struct Material
{
    sampler2D diffuse;
} material;


void main()
{
    if (texture(material.diffuse, in_.uv).a < 0.25) discard;
    // gl_FragDepth = gl_FragCoord.z;
}


#else // !ENABLE_ALPHA_TESTING


void main()
{
    // gl_FragDepth = gl_FragCoord.z;
}


#endif // ENABLE_ALPHA_TESTING
