#version 430 core

in flat uint  draw_id;
in flat uint  object_id;
in flat float specpower;
in      vec2  uv;
in      vec3  frag_pos;
in      mat3  TBN;

#ifndef MAX_TEXTURE_UNITS
#define MAX_TEXTURE_UNITS 3
#endif

uniform sampler2D samplers[MAX_TEXTURE_UNITS];

layout (location = 0) out vec3  out_normal;
layout (location = 1) out vec3  out_albedo;
layout (location = 2) out float out_specular;
layout (location = 3) out uint  out_object_id;


void main()
{
    const uint diffuse_id  = draw_id * 3 + 0;
    const uint specular_id = draw_id * 3 + 1;
    const uint normal_id   = draw_id * 3 + 2;

    const vec4  mat_diffuse  = texture(samplers[diffuse_id],  uv).rgba;
    const float mat_specular = texture(samplers[specular_id], uv).r;
    const vec3  mat_normal   = texture(samplers[normal_id],   uv).xyz;
    const vec3  normal_ts    = mat_normal * 2.0 - 1.0;
    const vec3  normal       = normalize(TBN * normal_ts);

#ifdef ENABLE_ALPHA_TESTING
    if (mat_diffuse.a < 0.5) discard;
#endif // ENABLE_ALPHA_TESTING

    out_normal    = gl_FrontFacing ? normal : -normal;
    out_albedo    = mat_diffuse.rgb;
    out_specular  = mat_specular.r;
    out_object_id = object_id;
}
