#pragma once
#include "LightCasters.hpp"
#include <entt/fwd.hpp>


namespace josh::imguihooks::registry {


class LightComponents {
private:
    PointLight plight_template_{ PointLight{
        .color = { 1.f, 1.f, 0.8f },
        .position = { 0.0f, 1.f, 0.f },
        .attenuation = Attenuation{
            .constant = 0.05f,
            .linear = 0.0f,
            .quadratic = 0.2f
        }
    }};

    bool plight_has_shadow_{ true };

public:
    void operator()(entt::registry& registry);
};


} // namespace josh::imguihooks::registry
