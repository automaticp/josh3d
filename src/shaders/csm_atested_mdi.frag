#version 460 core

in flat uint draw_id;
in      vec2 uv;

#ifndef MAX_TEXTURE_UNITS
#define MAX_TEXTURE_UNITS 1
#endif

uniform sampler2D samplers[MAX_TEXTURE_UNITS];


void main()
{
    if (texture(samplers[draw_id], uv).a < 0.25) discard;
//  gl_FragDepth = gl_FragCoord.z;
}
