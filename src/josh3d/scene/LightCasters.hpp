#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


/*
Scene definitons of light sources.

Positions/Orientations are to be inferred from Transforms.

TODO: Pairs of `color` and `irradiance`/`power` should probably
just be stored as a single `spectral_power`/`spectral_irradiance` vec3s,
and be decomposed into value=1 colors and power for UI.

NOTE: I might be wrong in how I use some of the terminology here.

Use `hdr_color()` to get actual spectral power/irradiance values for rendering.
*/
namespace josh {


struct AmbientLight {
    glm::vec3 color     { 1.f };
    float     irradiance{ 1.f };

    auto hdr_color() const noexcept
        -> glm::vec3
    {
        return color * irradiance;
    }
};


struct DirectionalLight {
    glm::vec3 color     { 1.f };
    // Irradiance received by a surface that is normal to the light direction, [W/m^2].
    float     irradiance{ 1.f };

    auto hdr_color() const noexcept
        -> glm::vec3
    {
        return color * irradiance;
    }
};


struct PointLight {
    glm::vec3 color{ 1.f };
    // Radiant power/flux of a point light source with HSV value == 1, [W].
    float     power{ 1.f };

    auto hdr_color() const noexcept
        -> glm::vec3
    {
        return color * power;
    }

    auto quadratic_attenuation() const noexcept
        -> float
    {
        constexpr float four_pi = 4.f * glm::pi<float>();
        return four_pi / power;
    }
};


/*
NOTE: Currently not supported in any way.
*/
struct SpotLight {
    glm::vec3   color{ 1.f };
    float       power{ 1.f };
    float       inner_cutoff_radians{ glm::radians(45.f) };
    float       outer_cutoff_radians{ glm::radians(50.f) };

    auto hdr_color() const noexcept
        -> glm::vec3
    {
        return color * power;
    }
};


} // namespace josh
