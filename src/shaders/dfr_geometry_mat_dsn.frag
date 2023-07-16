#version 330 core
layout (location = 0) out vec4 out_position_draw;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo_spec;

in vec2 tex_coords;
in mat3 TBN;
in vec3 frag_pos;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    float shininess;
} material;



void main() {
    vec4 tex_diffuse = texture(material.diffuse, tex_coords);

#ifdef ENABLE_ALPHA_TESTING
    if (tex_diffuse.a < 0.5) discard;
#endif // ENABLE_ALPHA_TESTING

    out_albedo_spec.rgb = tex_diffuse.rgb;
    out_albedo_spec.a = texture(material.specular, tex_coords).r;

    out_position_draw = vec4(frag_pos, 1.0);

    vec3 tangent_space_normal = texture(material.normal, tex_coords).rgb * 2.0 - 1.0;
    out_normal = vec4(normalize(TBN * tangent_space_normal), 1.0);

}
