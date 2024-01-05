#pragma once
#include "LightCasters.hpp"
#include <entt/fwd.hpp>


namespace josh::imguihooks::registry {


class LightComponents {
private:
    light::Point plight_template_{ light::Point{
        .color = { 1.f, 1.f, 0.8f },
        .position = { 0.0f, 1.f, 0.f },
        .attenuation = light::Attenuation{
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
