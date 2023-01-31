#version 330 core

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;
in vec4 frag_pos_light_space;

out vec4 frag_color;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;


uniform struct AmbientLight {
    vec3 color;
} ambient_light;

uniform struct DirectionalLight {
    vec3 color;
    vec3 direction;
} dir_light;


// uniform vec3 cam_pos;
uniform sampler2D shadow_map;
uniform vec2 shadow_bias_bounds;


float shadow_yield(vec4 frag_pos_light_space) {

    // Perspective divide to [-1, 1]
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    // To NDC [0, 1]
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.z > 1.0) {
        return 0.0;
    }

    float closest_depth = texture(shadow_map, proj_coords.xy).r;
    float frag_depth = proj_coords.z;

    float total_bias = max(
        shadow_bias_bounds.x,
        shadow_bias_bounds.y * (1.0 - dot(normal, normalize(dir_light.direction)))
    );

    return closest_depth < frag_depth - total_bias ? 1.0 : 0.0;
}


void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));


    vec3 normal_dir = normalize(normal);
    vec3 light_dir = normalize(dir_light.direction);

    float diffuse_alignment = max(dot(normal_dir, -light_dir), 0.0);


    vec3 result_color = vec3(0.0);

    result_color += ambient_light.color * tex_diffuse;
    result_color += dir_light.color * diffuse_alignment * tex_diffuse * (1.0 - shadow_yield(frag_pos_light_space));


    frag_color = vec4(result_color, 1.0);

}
