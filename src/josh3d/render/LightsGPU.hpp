#pragma once
#include "GPULayout.hpp"
#include "Math.hpp"


/*
GPU struct compatible definitions of light sources.
*/
namespace josh {


struct AmbientLightGPU {
    alignas(std430::align_vec3) vec3 color;
};


struct DirectionalLightGPU {
    alignas(std430::align_vec3) vec3 color;
    alignas(std430::align_vec3) vec3 direction;
};


struct PointLightGPU {
    alignas(std430::align_vec3) vec3 color;
    alignas(std430::align_vec3) vec3 position;
};


struct PointLightBoundedGPU {
    alignas(std430::align_vec3)  vec3  color;
    alignas(std430::align_vec3)  vec3  position;
    alignas(std430::align_float) float radius;
};


struct SpotLightGPU {
    alignas(std430::align_vec3)  vec3  color;
    alignas(std430::align_vec3)  vec3  position;
    alignas(std430::align_vec3)  vec3  direction;
    alignas(std430::align_float) float inner_cutoff_radians;
    alignas(std430::align_float) float outer_cutoff_radians;
};


} // namespace josh
