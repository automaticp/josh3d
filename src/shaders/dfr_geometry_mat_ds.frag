#version 330 core
layout (location = 0) out vec4 out_position_draw;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo_spec;

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;




void main() {

    out_position_draw = vec4(frag_pos, 1.0);

    out_normal = vec4(normal, 1.0);

    out_albedo_spec.rgb = texture(material.diffuse, tex_coords).rgb;
    out_albedo_spec.a   = texture(material.specular, tex_coords).r;

}
