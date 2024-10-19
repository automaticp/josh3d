#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "gbuffer.glsl"
#include "camera_ubo.glsl"
#include "shading.phong.glsl"
#define CASCADE_VIEWS_SSBO_BINDING 0
#include "shading.csm.glsl"


out vec4 frag_color;
in  vec2 tex_coords;


uniform GBuffer gbuffer;


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

    const PhongMaterial material = { gsample.albedo, gsample.specular, 128.0 };

    const vec3 frag_pos_ws = vec3(camera.inv_view * gsample.position_vs);
    const vec3 normal_dir  = normalize(gsample.normal);
    const vec3 view_dir    = normalize(camera.position_ws - frag_pos_ws);

    vec3 result_color = vec3(0.0);

    // Directional Light.
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

    // Ambient Light.
    if (use_ambient_occlusion) {
        const float ambient_occlusion = texture(tex_ambient_occlusion, tex_coords).r;
        result_color += alight.color * material.albedo * pow(1.0 - ambient_occlusion, ambient_occlusion_power);
    } else {
        result_color += alight.color * material.albedo;
    }


    frag_color = vec4(result_color, 1.0);
}




