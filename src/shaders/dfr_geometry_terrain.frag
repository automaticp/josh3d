#version 430 core

layout (location = 0) out vec4 out_position_draw;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo_spec;
layout (location = 3) out uint out_object_id;

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;

// uniform struct Material {
//     sampler2D diffuse;
//     sampler2D specular;
//     float shininess;
// } material;

uniform sampler2D test_color;

uniform uint object_id;


void main() {
    // vec4 tex_diffuse = texture(material.diffuse, tex_coords);

// #ifdef ENABLE_ALPHA_TESTING
//     if (tex_diffuse.a < 0.5) discard;
// #endif // ENABLE_ALPHA_TESTING

    // out_albedo_spec.rgb = tex_diffuse.rgb;
    // out_albedo_spec.a   = texture(material.specular, tex_coords).r;


    out_albedo_spec = vec4(vec3(texture(test_color, tex_coords).r), 0.2);

    out_position_draw = vec4(frag_pos, 1.0);

    out_normal = gl_FrontFacing ? vec4(normal, 1.0) : vec4(-normal, 1.0);

    out_object_id = object_id;
}
