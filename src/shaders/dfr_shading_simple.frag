#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"


in vec2  tex_coords;
out vec4 frag_color;


uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_albedo_spec;

uniform AmbientLight     ambient_light;
uniform DirectionalLight dir_light;

layout (std430, binding = 1) restrict readonly buffer PointLightBlock {
    PointLight point_lights[];
};

uniform vec3 cam_pos;




void main() {

    bool was_overdrawn = texture(tex_position_draw, tex_coords).a > 0.0;
    if (!was_overdrawn) { discard; }

    vec3  tex_diffuse  = texture(tex_albedo_spec, tex_coords).rgb;
    float tex_specular = texture(tex_albedo_spec, tex_coords).a;

    vec3 frag_pos = texture(tex_position_draw, tex_coords).rgb;
    vec3 normal   = texture(tex_normals,       tex_coords).rgb;


    vec3 normal_dir = normalize(normal);
    vec3 view_dir = normalize(cam_pos - frag_pos);

    vec3 result_color = vec3(0.0);

    // Point Lights
    {
        for (int i = 0; i < point_lights.length(); ++i) {
            const PointLight plight = point_lights[i];

            vec3 light_vec      = plight.position - frag_pos;
            vec3 light_dir      = normalize(light_vec);
            vec3 reflection_dir = reflect(-light_dir, normal_dir);

            float diffuse_alignment  = max(dot(normal_dir, light_dir),      0.0);
            float specular_alignment = max(dot(view_dir,   reflection_dir), 0.0);

            float distance_factor = get_attenuation_factor(plight.attenuation, length(light_vec));

            float diffuse_strength  = distance_factor * diffuse_alignment;
            float specular_strength = distance_factor * pow(specular_alignment, 128.0);
            // FIXME: Hardcoded shininess because it is not stored in the GBuffer ^.

            result_color += plight.color * diffuse_strength  * tex_diffuse;
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
