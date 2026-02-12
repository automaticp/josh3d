// #version 430 core
#ifndef SHADING_PHONG_GLSL
#define SHADING_PHONG_GLSL


struct PhongMaterial
{
    vec3  albedo;
    float specular;
    float shininess;
};

vec3 phong_brdf(
    vec3          irradiance,
    vec3          light_dir,
    vec3          view_dir,
    vec3          normal_dir,
    PhongMaterial material)
{
    const vec3 reflection_dir = reflect(-light_dir, normal_dir);

    const float diffuse_alignment  = max(dot(normal_dir, light_dir),      0.0);
    const float specular_alignment = max(dot(normal_dir, reflection_dir), 0.0);

    const float diffuse_strength  = diffuse_alignment;
    const float specular_strength = pow(specular_alignment, material.shininess);

    const vec3 diffuse_contribution  = irradiance * material.albedo   * diffuse_strength;
    const vec3 specular_contribution = irradiance * material.specular * specular_strength;

    return diffuse_contribution + specular_contribution;
}

/*
This is not a distribution function per-se, since you can't
just *evaluate* the distribution funcition at a set of parameters
to get a finite probability.
*/
vec3 blinn_phong_brdf(
    vec3          irradiance,
    vec3          light_dir,
    vec3          view_dir,
    vec3          normal_dir,
    PhongMaterial material)
{
    const vec3 halfway_dir = normalize(light_dir + view_dir);

    const float diffuse_alignment  = max(dot(normal_dir, light_dir),   0.0);
    const float specular_alignment = max(dot(normal_dir, halfway_dir), 0.0);

    const float diffuse_strength  = diffuse_alignment;
    const float specular_strength = pow(specular_alignment, material.shininess);

    const vec3 diffuse_contribution  = irradiance * material.albedo   * diffuse_strength;
    const vec3 specular_contribution = irradiance * material.specular * specular_strength;

    return diffuse_contribution + specular_contribution;
}


#endif
