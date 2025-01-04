#version 430 core

layout (location = 0) out vec3  out_normal;
layout (location = 1) out vec3  out_albedo;
layout (location = 2) out float out_specular;
layout (location = 3) out uint  out_object_id;

in Interface {
    vec2 uv;
    mat3 TBN;
    vec3 frag_pos;
} in_;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    float shininess;
} material;

uniform uint object_id;


void main() {
    vec4  mat_diffuse  = texture(material.diffuse,  in_.uv).rgba;
    float mat_specular = texture(material.specular, in_.uv).r;
    vec3  mat_normal   = texture(material.normal,   in_.uv).xyz;
    vec3  normal_ts    = mat_normal * 2.0 - 1.0;
    vec3  normal       = normalize(in_.TBN * normal_ts);

#ifdef ENABLE_ALPHA_TESTING
    if (mat_diffuse.a < 0.5) discard;
#endif // ENABLE_ALPHA_TESTING

    out_normal    = gl_FrontFacing ? normal : -normal;
    out_albedo    = mat_diffuse.rgb;
    out_specular  = mat_specular.r;
    out_object_id = object_id;
}
