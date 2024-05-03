#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "lights.glsl"
#include "utils.coordinates.glsl"


layout (location = 0) in vec3 in_pos;


struct PLight {
    vec3        color;
    vec3        position;
    float       radius;
    Attenuation attenuation;
};


out Interface {
    flat uint  plight_id;
    PLight     plight;
} out_;


layout (std430, binding = 0) restrict readonly
buffer PointLightsBlock {
    PLight point_lights[];
};


uniform mat4 view;
uniform mat4 proj;




void main() {
    uint   plight_id = gl_InstanceID;
    PLight plight    = point_lights[plight_id];

    vec4 pos_ws = vec4(plight.position + (in_pos * plight.radius), 1.0);
    vec4 pos_ps = proj * view * pos_ws;

    gl_Position    = pos_ps;
    out_.plight_id = plight_id;
    out_.plight    = plight;
}
