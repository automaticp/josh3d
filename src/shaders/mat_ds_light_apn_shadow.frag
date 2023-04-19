#version 430 core

// layout (std430, binding = 2) restrict readonly buffer point_light_depth_cubemap_layout {
//     samplerCube point_light_depth_cubemaps[];
// };
//
//     RIP 2023-2023

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


struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};


struct PointLight {
    vec3 color;
    vec3 position;
    Attenuation attenuation;
};


layout (std430, binding = 1) restrict readonly buffer point_light_with_shadows_block {
    PointLight point_lights_with_shadows[];
};

uniform samplerCubeArray point_light_shadow_maps;

uniform float point_light_z_far;
uniform vec2 point_shadow_bias_bounds;



layout (std430, binding = 2) restrict readonly buffer point_light_no_shadows_block {
    PointLight point_lights_no_shadows[];
};


uniform vec3 cam_pos;




float get_distance_factor(float light_distance, Attenuation att) {
    return 1.0 / (
        att.constant +
        att.linear * light_distance +
        att.quadratic * (light_distance * light_distance)
    );
}


float point_light_shadow_yield(vec3 frag_pos, PointLight plight, int shadow_map_idx) {
    vec3 frag_to_light = frag_pos - plight.position;
    // Normalized depth value in [0, 1]
    float closest_depth = texture(
        point_light_shadow_maps,
        vec4(frag_to_light, shadow_map_idx)
    ).r;
    // Transofrm back to [0, z_far]
    // FIXME: Isn't this supposed to be z_far - z_near as the coefficient?
    closest_depth *= point_light_z_far;

    float frag_depth = length(frag_to_light);

    float total_bias = max(
        point_shadow_bias_bounds.x,
        point_shadow_bias_bounds.y * (1.0 - dot(normal, normalize(frag_to_light)))
    );

    return closest_depth < frag_depth - total_bias ? 1.0 : 0.0;
}


vec3 add_point_light_illumination(vec3 in_color, PointLight plight,
    vec3 tex_diffuse, vec3 tex_specular,
    vec3 normal_dir, vec3 view_dir)
{
    vec3 light_vec = plight.position - frag_pos;
    vec3 light_dir = normalize(light_vec);
    vec3 reflection_dir = reflect(-light_dir, normal_dir);

    float diffuse_alignment = max(dot(normal_dir, light_dir), 0.0);
    float specular_alignment = max(dot(view_dir, reflection_dir), 0.0);

    float distance_factor =
        get_distance_factor(length(light_vec), plight.attenuation);

    float diffuse_strength = distance_factor * diffuse_alignment;
    float specular_strength =
        distance_factor * pow(specular_alignment, material.shininess);

    in_color += plight.color * diffuse_strength * tex_diffuse;
    in_color += plight.color * specular_strength * tex_specular;

    return in_color;
}


void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));

    vec3 normal_dir = normalize(normal);
    vec3 view_dir = normalize(cam_pos - frag_pos);

    vec3 result_color = vec3(0.0);

    // Point lights.
    {
        // With shadows.
        for (int i = 0; i < point_lights_with_shadows.length(); ++i) {
            const PointLight plight = point_lights_with_shadows[i];
            const float imllumination_factor =
                1.0 - point_light_shadow_yield(frag_pos, plight, i);

            // How bad is this branching?
            // Is it better to branch or to factor out the result?
            if (imllumination_factor == 0.0) continue;

            result_color = add_point_light_illumination(
                result_color, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }

        // Without shadows.
        for (int i = 0; i < point_lights_no_shadows.length(); ++i) {
            const PointLight plight = point_lights_no_shadows[i];

            result_color = add_point_light_illumination(
                result_color, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }
    }

    // Ambient light.
    result_color += ambient_light.color * tex_diffuse;

    frag_color = vec4(result_color, 1.0);

}
