#pragma once
#include <entt/entity/fwd.hpp>


namespace josh {


class ImGuiSelected {
public:
    bool display_model_matrix{ false };
    bool display_all_components{ false };

    ImGuiSelected(entt::registry& registry) : registry_{ registry } {}

    void display();

private:
    entt::registry& registry_;
};


} // namespace josh
