#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "camera_ubo.glsl"
#include "gbuffer.glsl"
#include "shading.lights.glsl"
#include "shading.phong.glsl"
#include "shading.point_shadows.glsl"
#define CASCADE_VIEWS_SSBO_BINDING 3
#include "shading.csm.glsl"


out vec4 frag_color;
in  vec2 tex_coords;


uniform GBuffer gbuffer;


layout (std430, binding = 1) restrict readonly
buffer PointLightWithShadowsBlock {
    PointLightBounded plights_with_shadows[];
};

layout (std430, binding = 2) restrict readonly
buffer PointLightNoShadowsBlock {
    PointLightBounded plights_no_shadows[];
};

uniform float                  plight_fade_start_fraction;
uniform samplerCubeArrayShadow psm_maps;
uniform PointShadowParams      psm_params;


uniform DirectionalLight     dlight;
uniform bool                 dlight_cast_shadows;
uniform sampler2DArrayShadow csm_maps;
uniform CascadeShadowParams  csm_params;


uniform AmbientLight alight;
uniform bool         use_ambient_occlusion = false;
uniform sampler2D    tex_ambient_occlusion;
uniform float        ambient_occlusion_power = 1.0;




void main() {

    GSample gsample = unpack_gbuffer(gbuffer, tex_coords, camera.z_near, camera.z_far, camera.inv_proj);

    if (!gsample.drawn) discard;

    const PhongMaterial material = { gsample.albedo, gsample.specular, 128.0 }; // Hardcoded shininess. Sad.


    const vec3 frag_pos_ws = vec3(camera.inv_view * gsample.position_vs);
    const vec3 normal_dir  = normalize(gsample.normal);
    const vec3 view_dir    = normalize(camera.position_ws - frag_pos_ws);


    vec3 result_color = vec3(0.0);

    // Point lights.
    {
        // With shadows.
        for (int i = 0; i < plights_with_shadows.length(); ++i) {
            const PointLightBounded plight         = plights_with_shadows[i];
            const float             z_far          = plight.radius;
            const vec3              light_to_frag  = frag_pos_ws - plight.position;
            const vec3              light_dir      = -normalize(light_to_frag);
            const float             light_distance = length(light_to_frag);

            if (light_distance > plight.radius) continue;

            const float obscurance = point_light_shadow_obscurance(psm_maps, i, psm_params, light_to_frag, normal_dir, z_far);

            if (obscurance == 1.0) continue; // I see about a 0.2 ms difference on my iGPU when looking at a highly shaded area.

            const vec3  surface_irradiance  = point_light_irradiance(plight.color, light_distance);
            const vec3  outgoing_irradiance = blinn_phong_brdf(surface_irradiance, light_dir, view_dir, normal_dir, material);
            const float edge_fade_factor    = point_light_volume_edge_fade(plight.radius, light_distance, plight_fade_start_fraction);

            result_color += outgoing_irradiance * edge_fade_factor * (1.0 - obscurance);
        }

        // Without shadows.
        for (int i = 0; i < plights_no_shadows.length(); ++i) {
            const PointLightBounded plight        = plights_no_shadows[i];
            const vec3              light_to_frag = frag_pos_ws - plight.position;
            const vec3              light_dir     = -normalize(light_to_frag);
            const float             light_distance = length(light_to_frag);

            if (light_distance > plight.radius) continue;

            const vec3  surface_irradiance  = point_light_irradiance(plight.color, light_distance);
            const vec3  outgoing_irradiance = blinn_phong_brdf(surface_irradiance, light_dir, view_dir, normal_dir, material);
            const float edge_fade_factor    = point_light_volume_edge_fade(plight.radius, light_distance, plight_fade_start_fraction);

            result_color += outgoing_irradiance * edge_fade_factor;
        }
    }

    // Directional light.
    {
        const vec3 surface_irradiance = dlight.color;
        const vec3 light_dir          = -normalize(dlight.direction);

        const vec3 outgoing_irradiance = blinn_phong_brdf(surface_irradiance, light_dir, view_dir, normal_dir, material);

        if (dlight_cast_shadows) {

            const float obscurance = directional_light_shadow_obscurance(csm_maps, csm_params, frag_pos_ws, normal_dir);
            result_color += outgoing_irradiance * (1.0 - obscurance);

        } else /* no shadow */ {

            result_color += outgoing_irradiance;

        }
    }

    // Ambient light.
    if (use_ambient_occlusion) {
        const float ambient_occlusion = texture(tex_ambient_occlusion, tex_coords).r;
        result_color += alight.color * material.albedo * pow(1.0 - ambient_occlusion, ambient_occlusion_power);
    } else {
        result_color += alight.color * material.albedo;
    }


    frag_color = vec4(result_color, 1.0);
}

