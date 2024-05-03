#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"

layout (location = 0) in vec3 in_pos;

out flat vec3 light_color;
out flat uint light_id;


struct PLightParams {
    vec3  position;
    float scale;
    vec3  color;
    uint  id;
};


layout (std430, binding = 0) restrict readonly
buffer PLightParamsBlock {
    PLightParams plight_params[];
};




void main() {
    PLightParams plight = plight_params[gl_InstanceID];

    vec3 pos_ws = plight.position + (plight.scale * in_pos);

    gl_Position = camera.projview * vec4(pos_ws, 1.0);
    light_color = plight.color;
    light_id    = plight.id;
}
