#version 330 core

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;

out vec4 frag_color;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;

uniform struct AmbientLight {
    vec3 color;
} ambient_light;



void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));

    vec3 result_color = ambient_light.color * tex_diffuse;

    frag_color = vec4(result_color, 1.0);

}
