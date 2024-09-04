#pragma once
#include "Layout.hpp"
#include <glm/glm.hpp>


/*
GPU struct compatible definitions of light sources.
*/
namespace josh {


struct AmbientLightGPU {
    alignas(std430::align_vec3) glm::vec3 color;
};


struct DirectionalLightGPU {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 direction;
};


struct AttenuationGPU {
    alignas(std430::align_float) float constant;
    alignas(std430::align_float) float linear;
    alignas(std430::align_float) float quadratic;
};


struct PointLightGPU {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    AttenuationGPU                        attenuation;
};


struct PointLightBoundedGPU {
    alignas(std430::align_vec3)  glm::vec3 color;
    alignas(std430::align_vec3)  glm::vec3 position;
    alignas(std430::align_float) float     radius;
    AttenuationGPU                         attenuation;
};


struct SpotLightGPU {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    alignas(std430::align_vec3) glm::vec3 direction;
    AttenuationGPU                        attenuation;
    alignas(std430::align_float) float    inner_cutoff_radians;
    alignas(std430::align_float) float    outer_cutoff_radians;
};


} // namespace josh
