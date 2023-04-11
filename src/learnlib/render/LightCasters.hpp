#pragma once
#include <glm/glm.hpp>


namespace learn {


namespace light {


struct Ambient {
    glm::vec3 color;
};

struct Directional {
    glm::vec3 color;
    glm::vec3 direction;
};

struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct Point {
    glm::vec3 color;
    glm::vec3 position;
    Attenuation attenuation;
};

struct Spotlight {
    glm::vec3 color;
    glm::vec3 position;
    glm::vec3 direction;
    Attenuation attenuation;
    float inner_cutoff_radians;
    float outer_cutoff_radians;
};


} // namespace light


} // namespace learn
