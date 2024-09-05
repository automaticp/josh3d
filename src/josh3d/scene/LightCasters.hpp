#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>


/*
Scene definitons of light sources.

Positions/Orientations are to be inferred from Transforms.
*/
namespace josh {


struct AmbientLight {
    glm::vec3 color;
};


struct DirectionalLight {
    glm::vec3 color;
};


struct Attenuation {
    float constant;
    float linear;
    float quadratic;

    float get_attenuation(float distance) const noexcept {
        return 1.f / (
            constant +
            linear    * distance +
            quadratic * distance * distance
        );
    }
};


struct PointLight {
    glm::vec3   color;
    Attenuation attenuation;
};


struct SpotLight {
    glm::vec3   color;
    Attenuation attenuation;
    float       inner_cutoff_radians;
    float       outer_cutoff_radians;
};


} // namespace josh
