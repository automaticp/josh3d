#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "gbuffer.glsl"
#include "camera_ubo.glsl"
#include "shading.phong.glsl"
#include "shading.lights.glsl"
#include "shading.point_shadows.glsl"


out vec4 frag_color;


in Interface {
    flat uint         plight_id;
    PointLightBounded plight;
} in_;


uniform GBuffer gbuffer;


uniform float                  plight_fade_start_fraction;
uniform samplerCubeArrayShadow psm_maps;
uniform PointShadowParams      psm_params;




void main() {
    const vec2 uv = gl_FragCoord.xy / vec2(textureSize(gbuffer.tex_normals, 0));

    GSample gsample = unpack_gbuffer(gbuffer, uv, camera.z_near, camera.z_far, camera.inv_proj);

    if (!gsample.drawn) discard;

    const PhongMaterial material = { gsample.albedo, gsample.specular, 128.0 };

    const vec3  frag_pos_ws    = vec3(camera.inv_view * gsample.position_vs);
    const vec3  normal_dir     = normalize(gsample.normal);
    const vec3  view_dir       = normalize(camera.position_ws - frag_pos_ws);
    const vec3  light_to_frag  = frag_pos_ws - in_.plight.position;
    const vec3  light_dir      = -normalize(light_to_frag);
    const float light_distance = length(light_to_frag);
    const float z_far          = in_.plight.radius;

    const float obscurance = point_light_shadow_obscurance(psm_maps, int(in_.plight_id), psm_params, light_to_frag, normal_dir, z_far);

    if (obscurance == 1.0) discard;

    const vec3  surface_irradiance  = point_light_irradiance(in_.plight.color, light_distance);
    const vec3  outgoing_irradiance = blinn_phong_brdf(surface_irradiance, light_dir, view_dir, normal_dir, material);
    const float edge_fade_factor    = point_light_volume_edge_fade(in_.plight.radius, light_distance, plight_fade_start_fraction);

    const vec3 result_color = outgoing_irradiance * edge_fade_factor * (1.0 - obscurance);

    frag_color = vec4(result_color, 1.0);
}


