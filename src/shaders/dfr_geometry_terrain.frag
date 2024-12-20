#version 430 core

layout (location = 0) out vec3  out_normal;
layout (location = 1) out vec3  out_albedo;
layout (location = 2) out float out_specular;
layout (location = 3) out uint  out_object_id;

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;

// uniform struct Material {
//     sampler2D diffuse;
//     sampler2D specular;
//     float shininess;
// } material;

uniform sampler2D test_color;
uniform uint      object_id;


void main() {
    vec4 mat_diffuse = texture(test_color, tex_coords);

// #ifdef ENABLE_ALPHA_TESTING
//     if (tex_diffuse.a < 0.5) discard;
// #endif // ENABLE_ALPHA_TESTING

    out_normal    = gl_FrontFacing ? normal : -normal;
    out_albedo    = vec3(mat_diffuse.r);
    out_specular  = 0.2;
    out_object_id = object_id;
}
