#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_tangent;

struct InstanceData
{
    mat4  model;
    mat3  normal_model;
    uint  object_id;
    float specpower;
};

layout (std430, binding = 0) restrict readonly
buffer InstanceDataBlock
{
    InstanceData instances[];
};

out flat uint  draw_id;
out flat uint  object_id;
out flat float specpower;
out      vec2  uv;
out      vec3  frag_pos;
out      mat3  TBN;


void main()
{
    draw_id = gl_DrawID;
    const InstanceData data = instances[draw_id];
    const mat4 model        = data.model;
    const mat3 normal_model = data.normal_model;

    // Gram-Schmidt renormalization.
    vec3 T = normalize(normal_model * in_tangent);
    vec3 N = normalize(normal_model * in_normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    object_id   = data.object_id;
    specpower   = data.specpower;
    uv          = in_uv;
    frag_pos    = vec3(model * vec4(in_pos, 1.0));
    TBN         = mat3(T, B, N);
    gl_Position = camera.projview * vec4(frag_pos, 1.0);
}
