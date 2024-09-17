#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "camera_ubo.glsl"
#include "utils.coordinates.glsl"


layout (location = 0) in vec3 in_pos;


out Interface {
    flat uint         plight_id;
    PointLightBounded plight;
} out_;


layout (std430, binding = 0) restrict readonly
buffer PointLightsBlock {
    PointLightBounded point_lights[];
};




void main() {
    const uint              plight_id = gl_InstanceID;
    const PointLightBounded plight    = point_lights[plight_id];

    const vec4 pos_ws = vec4(plight.position + (in_pos * plight.radius), 1.0);
    const vec4 pos_cs = camera.projview * pos_ws;

    gl_Position    = pos_cs;
    out_.plight_id = plight_id;
    out_.plight    = plight;
}
