#pragma once
#include "Layout.hpp"
#include <glm/glm.hpp>

namespace learn {


namespace light {


struct Ambient {
    alignas(layout::base_alignment_of_vec3) glm::vec3 color;
};

struct Directional {
    alignas(layout::base_alignment_of_vec3) glm::vec3 color;

    alignas(layout::base_alignment_of_vec3) glm::vec3 direction;
};

struct Attenuation {
    alignas(layout::base_alignment_of_float) float constant;
    alignas(layout::base_alignment_of_float) float linear;
    alignas(layout::base_alignment_of_float) float quadratic;
};

struct Point {
    alignas(layout::base_alignment_of_vec3) glm::vec3 color;
    alignas(layout::base_alignment_of_vec3) glm::vec3 position;
    Attenuation attenuation;
};

struct Spotlight {
    alignas(layout::base_alignment_of_vec3) glm::vec3 color;
    alignas(layout::base_alignment_of_vec3) glm::vec3 position;
    alignas(layout::base_alignment_of_vec3) glm::vec3 direction;
    Attenuation attenuation;
    alignas(layout::base_alignment_of_float) float inner_cutoff_radians;
    alignas(layout::base_alignment_of_float) float outer_cutoff_radians;
};


} // namespace light


} // namespace learn
