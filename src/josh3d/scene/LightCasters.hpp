#pragma once
#include "detail/Active.hpp"
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


struct ActiveAmbientLight {
    entt::entity entity{ entt::null };
};


[[nodiscard]] inline auto get_active_ambient_light(entt::registry& registry)
    -> entt::handle
{
    return detail::get_active<ActiveAmbientLight, AmbientLight>(registry);
}

[[nodiscard]] inline auto get_active_ambient_light(const entt::registry& registry)
    -> entt::const_handle
{
    return detail::get_active<ActiveAmbientLight, AmbientLight>(registry);
}




struct DirectionalLight {
    glm::vec3 color;
};


struct ActiveDirectionalLight {
    entt::entity entity{ entt::null };
};


[[nodiscard]] inline auto get_active_directional_light(entt::registry& registry)
    -> entt::handle
{
    return detail::get_active<ActiveDirectionalLight, DirectionalLight>(registry);
}

[[nodiscard]] inline auto get_active_directional_light(const entt::registry& registry)
    -> entt::const_handle
{
    return detail::get_active<ActiveDirectionalLight, DirectionalLight>(registry);
}




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
