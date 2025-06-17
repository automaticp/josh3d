#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL
// #version 430 core
// #extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"


struct GBuffer
{
    sampler2D tex_depth;
    sampler2D tex_normals;
    sampler2D tex_albedo;
    sampler2D tex_specular;
};

vec3 unpack_gbuffer_albedo(GBuffer gbuffer, vec2 uv)
{
    return textureLod(gbuffer.tex_albedo, uv, 0).rgb;
}

float unpack_gbuffer_specular(GBuffer gbuffer, vec2 uv)
{
    return textureLod(gbuffer.tex_specular, uv, 0).r;
}

vec3 unpack_gbuffer_normals(GBuffer gbuffer, vec2 uv)
{
    return textureLod(gbuffer.tex_normals, uv, 0).xyz;
}

float unpack_gbuffer_depth(GBuffer gbuffer, vec2 uv)
{
    return textureLod(gbuffer.tex_depth, uv, 0).r;
}

vec4 unpack_gbuffer_position_vs(GBuffer gbuffer, vec2 uv, float z_near, float z_far, mat4 inv_proj)
{
    float screen_z = textureLod(gbuffer.tex_depth, uv, 0).r;
    vec3  pos_nss  = vec3(uv, screen_z);
    vec4  pos_vs   = nss_to_vs(pos_nss, z_near, z_far, inv_proj);
    return pos_vs;
}

struct GSample
{
    vec4  position_vs;
    bool  drawn;
    vec3  normal;
    vec3  albedo;
    float specular;
};

GSample unpack_gbuffer(
    GBuffer gbuffer,
    vec2    uv,
    float   z_near,
    float   z_far,
    mat4    inv_proj)
{
    float screen_z = textureLod(gbuffer.tex_depth, uv, 0).r;
    vec4  pos_vs   = nss_to_vs(vec3(uv, screen_z), z_near, z_far, inv_proj);
    bool  drawn    = screen_z < 1.0;
    vec3  normal   = unpack_gbuffer_normals (gbuffer, uv);
    vec3  albedo   = unpack_gbuffer_albedo  (gbuffer, uv);
    float specular = unpack_gbuffer_specular(gbuffer, uv);

    return GSample(
        pos_vs,
        drawn,
        normal,
        albedo,
        specular
    );
}

#endif
