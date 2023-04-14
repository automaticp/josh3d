#version 430 core

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

uniform struct DirectionalLight {
    vec3 color;
    vec3 direction;
} dir_light;


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

// FIXME: Do not fix buffer bindings, rely on layout names
layout (std430, binding = 1) restrict readonly buffer point_light_layout {
    PointLight point_lights[];
};


uniform vec3 cam_pos;


float get_distance_factor(float light_distance, Attenuation att) {
    return 1.0 / (
        att.constant +
        att.linear * light_distance +
        att.quadratic * (light_distance * light_distance)
    );
}


void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));

    vec3 normal_dir = normalize(normal);
    vec3 view_dir = normalize(cam_pos - frag_pos);

    vec3 result_color = vec3(0.0);

    // Point Lights
    {
        for (int i = 0; i < point_lights.length(); ++i) {
            const PointLight plight = point_lights[i];

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

            result_color += plight.color * diffuse_strength * tex_diffuse;
            result_color += plight.color * specular_strength * tex_specular;
        }
    }

    // Directional Light
    {
        vec3 light_dir = normalize(dir_light.direction);
        float diffuse_alignment = max(dot(normal_dir, -light_dir), 0.0);
        result_color += dir_light.color * diffuse_alignment * tex_diffuse;
    }

    // Ambient Light
    result_color += ambient_light.color * tex_diffuse;

    frag_color = vec4(result_color, 1.0);

}
