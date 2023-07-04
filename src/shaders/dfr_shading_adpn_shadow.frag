#version 430 core

in vec2 tex_coords;

out vec4 frag_color;


struct AmbientLight {
    vec3 color;
};

struct DirectionalLight {
    vec3 color;
    vec3 direction;
};


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



uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_albedo_spec;

uniform AmbientLight ambient_light;
uniform DirectionalLight dir_light;


layout (std430, binding = 1) restrict readonly buffer point_light_with_shadows_block {
    PointLight point_lights_with_shadows[];
};

layout (std430, binding = 2) restrict readonly buffer point_light_no_shadows_block {
    PointLight point_lights_no_shadows[];
};


struct DirShadow {
    sampler2D map;
    vec2  bias_bounds;
    mat4  projection_view;
    bool  do_cast;
    int   pcf_samples;
    float pcf_offset;
};


struct PointShadow {
    samplerCubeArray maps;
    vec2  bias_bounds;
    float z_far;
    int   pcf_samples;
    float pcf_offset;
};


uniform DirShadow dir_shadow;
uniform PointShadow point_shadow;


uniform vec3 cam_pos;






float get_distance_factor(float light_distance, Attenuation att);


float point_light_shadow_yield(vec3 frag_pos, vec3 normal,
    PointLight plight, int shadow_map_idx);


vec3 add_point_light_illumination(vec3 in_color, vec3 frag_pos,
    float illumination_factor, PointLight plight,
    vec3 tex_diffuse, float tex_specular,
    vec3 normal_dir, vec3 view_dir);


float dir_light_shadow_yield(vec3 frag_pos, vec3 normal);




void main() {

    // Exit early if no pixels were written in GBuffer.
    bool was_overdrawn =
        texture(tex_position_draw, tex_coords).a > 0.0;
    if (!was_overdrawn) { discard; }

    // Unpack GBuffer.
    vec3 tex_diffuse = texture(tex_albedo_spec, tex_coords).rgb;
    float tex_specular = texture(tex_albedo_spec, tex_coords).a;

    vec3 frag_pos = texture(tex_position_draw, tex_coords).rgb;

    vec3 normal = texture(tex_normals, tex_coords).rgb;


    // Apply lighting and shadows.
    vec3 normal_dir = normalize(normal);
    vec3 view_dir = normalize(cam_pos - frag_pos);

    vec3 result_color = vec3(0.0);

    // Point lights.
    {
        // With shadows.
        for (int i = 0; i < point_lights_with_shadows.length(); ++i) {
            const PointLight plight = point_lights_with_shadows[i];
            const float illumination_factor =
                1.0 - point_light_shadow_yield(frag_pos, normal, plight, i);

            // How bad is this branching?
            // Is it better to branch or to factor out the result?
            if (illumination_factor == 0.0) continue;

            result_color = add_point_light_illumination(
                result_color, frag_pos,
                illumination_factor, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }

        // Without shadows.
        for (int i = 0; i < point_lights_no_shadows.length(); ++i) {
            const PointLight plight = point_lights_no_shadows[i];

            result_color = add_point_light_illumination(
                result_color, frag_pos,
                1.0, plight,
                tex_diffuse, tex_specular,
                normal_dir, view_dir
            );
        }
    }

    // Directional light.
    {
        const vec3 light_dir = normalize(dir_light.direction);
        const vec3 reflection_dir = reflect(light_dir, normal_dir);

        const float diffuse_alignment =
            max(dot(normal_dir, -light_dir), 0.0);

        const float specular_alignment =
            max(dot(view_dir, reflection_dir), 0.0);

        const float diffuse_strength = diffuse_alignment;

        // FIXME: Hardcoded shininess because it is not stored
        // in the GBuffer.
        const float specular_strength =
            pow(specular_alignment, 128.0);

        const float illumination_factor =
            dir_shadow.do_cast ?
                (1.0 - dir_light_shadow_yield(frag_pos, normal)) : 1.0;

        result_color +=
            dir_light.color * diffuse_strength *
            tex_diffuse * illumination_factor;

        result_color +=
            dir_light.color * specular_strength *
            tex_specular.r * illumination_factor;
    }


    // Ambient light.
    result_color += ambient_light.color * tex_diffuse;

    frag_color = vec4(result_color, 1.0);


}




float get_distance_factor(float light_distance, Attenuation att) {
    return 1.0 / (
        att.constant +
        att.linear * light_distance +
        att.quadratic * (light_distance * light_distance)
    );
}




float point_light_shadow_yield(vec3 frag_pos, vec3 normal,
    PointLight plight, int shadow_map_idx)
{
    const vec3 frag_to_light = frag_pos - plight.position;

    const float frag_depth = length(frag_to_light);

    const float total_bias = max(
        point_shadow.bias_bounds[0],
        point_shadow.bias_bounds[1] *
            (1.0 - dot(normal, normalize(frag_to_light)))
    );


    // Do PCF sampling.

    float yield = 0.0;

    const int pcf_samples = point_shadow.pcf_samples;
    const float pcf_offset = point_shadow.pcf_offset;

    for (int x = -pcf_samples; x <= pcf_samples; ++x) {
        for (int y = -pcf_samples; y <= pcf_samples; ++y) {
            for (int z = -pcf_samples; z <= pcf_samples; ++z) {

                const vec3 sample_dir = frag_to_light +
                    pcf_offset * vec3(x, y, z) / pcf_samples;

                // Normalized depth value in [0, 1]
                float pcf_depth = texture(point_shadow.maps,
                    vec4(sample_dir, shadow_map_idx)).r;

                // Transofrm back to [0, z_far]
                pcf_depth *= point_shadow.z_far;


                yield +=
                    frag_depth - total_bias > pcf_depth ? 1.0 : 0.0;

            }
        }
    }

    const int pcf_width = 1 + 2 * pcf_samples;

    return yield / (pcf_width * pcf_width * pcf_width);
}




vec3 add_point_light_illumination(vec3 in_color, vec3 frag_pos,
    float illumination_factor, PointLight plight,
    vec3 tex_diffuse, float tex_specular,
    vec3 normal_dir, vec3 view_dir)
{
    vec3 light_vec = plight.position - frag_pos;
    vec3 light_dir = normalize(light_vec);

    vec3 halfway_dir = normalize(light_dir + view_dir);

    float diffuse_alignment = max(dot(normal_dir, light_dir), 0.0);
    float specular_alignment = max(dot(normal_dir, halfway_dir), 0.0);

    float distance_factor =
        get_distance_factor(length(light_vec), plight.attenuation);

    float diffuse_strength = distance_factor * diffuse_alignment;
    float specular_strength =
        distance_factor * pow(specular_alignment, 128.0);

    in_color +=
        plight.color * diffuse_strength *
        tex_diffuse * illumination_factor;
    in_color +=
        plight.color * specular_strength *
        tex_specular * illumination_factor;

    return in_color;
}




float dir_light_shadow_yield(vec3 frag_pos, vec3 normal) {

    vec4 frag_pos_light_space =
        dir_shadow.projection_view * vec4(frag_pos, 1.0);

    // Perspective divide to [-1, 1]
    vec3 proj_coords =
        frag_pos_light_space.xyz / frag_pos_light_space.w;

    // To NDC [0, 1]
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.z > 1.0) {
        return 0.0;
    }

    const float frag_depth = proj_coords.z;

    const float total_bias = max(
        dir_shadow.bias_bounds[0],
        dir_shadow.bias_bounds[1] *
            (1.0 - dot(normal, normalize(dir_light.direction)))
    );


    // Do PCF sampling.

    float yield = 0.0;

    const vec2 texel_size = 1.0 / textureSize(dir_shadow.map, 0);
    const int pcf_samples = dir_shadow.pcf_samples;

    for (int x = -pcf_samples; x <= pcf_samples; ++x) {
        for (int y = -pcf_samples; y <= pcf_samples; ++y) {

            const vec2 offset =
                vec2(x, y) * texel_size * dir_shadow.pcf_offset;

            const float pcf_depth =
                texture(dir_shadow.map, proj_coords.xy + offset).r;

            yield +=
                frag_depth - total_bias > pcf_depth ? 1.0 : 0.0;
        }
    }

    // Size of N dimension for NxN PCF kernel.
    const int pcf_width = 1 + 2 * pcf_samples;

    // Normalize and return.
    return yield / (pcf_width * pcf_width);
}


