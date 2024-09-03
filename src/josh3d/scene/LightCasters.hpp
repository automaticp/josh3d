#pragma once
#include "Layout.hpp"
#include <glm/glm.hpp>


namespace josh {
// TODO: Separate scene representation of lights from the GPU structs.
// TODO: Also remove position/orientation in scene repr. and use the Transform.

struct AmbientLight {
    alignas(std430::align_vec3) glm::vec3 color;
};


struct DirectionalLight {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 direction;
};


struct Attenuation {
    alignas(std430::align_float) float constant;
    alignas(std430::align_float) float linear;
    alignas(std430::align_float) float quadratic;

    float get_attenuation(float distance) const noexcept {
        return 1.f / (
            constant +
            linear    * distance +
            quadratic * distance * distance
        );
    }
};


struct PointLight {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    Attenuation attenuation;
};


struct SpotLight {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    alignas(std430::align_vec3) glm::vec3 direction;
    Attenuation attenuation;
    alignas(std430::align_float) float inner_cutoff_radians;
    alignas(std430::align_float) float outer_cutoff_radians;
};


} // namespace josh
