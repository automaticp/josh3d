#version 330 core

// layout (std430, binding = 2) restrict readonly buffer point_light_depth_cubemap_layout {
//     samplerCube point_light_depth_cubemaps[];
// };
//
//     RIP 2023-2023

// Not actually 'apn', but just an 'ap1' for now

in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;

out vec4 frag_color;

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;


struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};


uniform struct PointLight {
    vec3 color;
    vec3 position;
    Attenuation attenuation;
} point_light;

uniform struct AmbientLight {
    vec3 color;
} ambient_light;

uniform vec3 cam_pos;

uniform samplerCube point_shadow_map;
uniform float point_light_z_far; // Is this the same far plane?
uniform vec2 point_shadow_bias_bounds;

float get_distance_factor(float light_distance, Attenuation att) {
    return 1.0 / (
        att.constant +
        att.linear * light_distance +
        att.quadratic * (light_distance * light_distance)
    );
}


float point_light_shadow_yield(vec3 frag_pos) {
    vec3 frag_to_light = frag_pos - point_light.position;
    // Normalized depth value in [0, 1]
    float closest_depth = texture(point_shadow_map, frag_to_light).r;
    // Transofrm back to [0, z_far]
    // FIXME: Isn't this supposed to be z_far - z_near as the coefficient?
    // FIXME: Is this supposed to be a z_far for shadow or for the scene?
    closest_depth *= point_light_z_far;

    float frag_depth = length(frag_to_light);

    float total_bias = max(
        point_shadow_bias_bounds.x,
        point_shadow_bias_bounds.y * (1.0 - dot(normal, normalize(frag_to_light)))
    );

    return closest_depth < frag_depth - total_bias ? 1.0 : 0.0;
}


void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));

    vec3 normal_dir = normalize(normal);
    vec3 light_vec = point_light.position - frag_pos;
    vec3 light_dir = normalize(light_vec);
    vec3 view_dir = normalize(cam_pos - frag_pos);

    vec3 reflection_dir = reflect(-light_dir, normal_dir);
    float light_distance = length(light_vec);

    float diffuse_alignment = max(dot(normal_dir, light_dir), 0.0);
    float specular_alignment = max(dot(view_dir, reflection_dir), 0.0);

    float distance_factor = get_distance_factor(light_distance, point_light.attenuation);

    float diffuse_strength = distance_factor * diffuse_alignment;
    float specular_strength = distance_factor * pow(specular_alignment, material.shininess);


    vec3 result_color = vec3(0.0);

    result_color += ambient_light.color * tex_diffuse;

    float shadow_factor = 1.0 - point_light_shadow_yield(frag_pos);

    result_color += shadow_factor * point_light.color * diffuse_strength * tex_diffuse;
    result_color += shadow_factor * point_light.color * specular_strength * tex_specular;


    frag_color = vec4(result_color, 1.0);

}
