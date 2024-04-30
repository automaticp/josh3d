#pragma once
#include "Layout.hpp"
#include <glm/glm.hpp>

namespace josh {


namespace light {


struct Ambient {
    alignas(std430::align_vec3) glm::vec3 color;
};

struct Directional {
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

struct Point {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    Attenuation attenuation;
};

struct Spotlight {
    alignas(std430::align_vec3) glm::vec3 color;
    alignas(std430::align_vec3) glm::vec3 position;
    alignas(std430::align_vec3) glm::vec3 direction;
    Attenuation attenuation;
    alignas(std430::align_float) float inner_cutoff_radians;
    alignas(std430::align_float) float outer_cutoff_radians;
};


} // namespace light


} // namespace josh
