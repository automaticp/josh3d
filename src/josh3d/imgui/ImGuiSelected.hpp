#pragma once
#include <entt/entity/fwd.hpp>


namespace josh {


class ImGuiSelected {
private:
    entt::registry& registry_;

public:
    ImGuiSelected(entt::registry& registry)
        : registry_{ registry }
    {}

    void display();
};


} // namespace josh
