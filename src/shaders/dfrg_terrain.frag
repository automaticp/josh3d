#version 430 core

in vec2 uv;
in vec3 normal;
in vec3 frag_pos;

uniform sampler2D test_color;
uniform uint      object_id;

layout (location = 0) out vec3  out_normal;
layout (location = 1) out vec3  out_albedo;
layout (location = 2) out float out_specular;
layout (location = 3) out uint  out_object_id;


void main()
{
    const vec4 mat_diffuse = texture(test_color, uv);

    out_normal    = gl_FrontFacing ? normal : -normal;
    out_albedo    = vec3(mat_diffuse.r);
    out_specular  = 0.2;
    out_object_id = object_id;
}
