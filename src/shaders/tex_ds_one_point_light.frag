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

// uniform Material material;
// uniform PointLight point_light;

uniform vec3 cam_pos;





struct LightAlignmentDistanceFactors {
    float diffuse_alignment;
    float specular_alignment;
    float distance_factor;
};

LightAlignmentDistanceFactors
get_light_alignment_distance_factors(
    vec3 normal_dir, vec3 light_vec, vec3 view_dir, Attenuation att)
{

    LightAlignmentDistanceFactors ladf;

    vec3 light_dir = normalize(light_vec);
    vec3 reflection_dir = reflect(-light_dir, normal_dir);
    float light_distance = length(light_vec);

    ladf.diffuse_alignment = max(dot(normal_dir, light_dir), 0.0);
    ladf.specular_alignment = max(dot(view_dir, reflection_dir), 0.0);

    ladf.distance_factor =
        1.0 / (
            att.constant +
            att.linear * light_distance +
            att.quadratic * (light_distance * light_distance)
        );

    return ladf;
}


// Strength after accounting for alignment and distance
struct LightComponentStrength {
    float ambient; // Why is ambient part of any light?
    float diffuse;
    float specular;
};

LightComponentStrength
get_light_component_strength(const LightAlignmentDistanceFactors ladf) {

    LightComponentStrength lcs;
    lcs.ambient = 0.1; // ???
    lcs.diffuse = ladf.diffuse_alignment * ladf.distance_factor;
    lcs.specular = pow(ladf.specular_alignment, material.shininess) * ladf.distance_factor;

    return lcs;
}



vec3 apply_light(
    vec3 light_color, const LightComponentStrength lcs,
    vec3 tex_diffuse, vec3 tex_specular)
{

    vec3 ambient = lcs.ambient * tex_diffuse;
    vec3 diffuse = lcs.diffuse * tex_diffuse;
    vec3 specular = lcs.specular * tex_specular;

    return light_color * (ambient + diffuse + specular);
}





void main() {
    vec3 tex_diffuse = vec3(texture(material.diffuse, tex_coords));
    vec3 tex_specular = vec3(texture(material.specular, tex_coords));

    vec3 normal_dir = normalize(normal);
    vec3 light_vec = point_light.position - frag_pos;
    vec3 view_dir = normalize(cam_pos - frag_pos);

    LightAlignmentDistanceFactors ladf =
        get_light_alignment_distance_factors(
            normal_dir, light_vec, view_dir, point_light.attenuation
        );

    LightComponentStrength lcs = get_light_component_strength(ladf);

    vec3 result_color =
        apply_light(point_light.color, lcs, tex_diffuse, tex_specular);

    frag_color = vec4(result_color, 1.0);

}
