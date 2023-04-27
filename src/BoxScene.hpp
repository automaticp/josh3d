#pragma once
#include "AmbientBackgroundStage.hpp"
#include "AssimpModelLoader.hpp"
#include "Camera.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "MaterialDSMultilightStage.hpp"
#include "PointLightSourceBoxStage.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include <glfwpp/window.h>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <array>
#include <iterator>
#include <string>
#include <vector>


namespace leakslearn {

using namespace learn;


class BoxScene3 {
private:
    glfw::Window& window_;

    entt::registry registry_;

    Camera cam_{ { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };

    SimpleInputBlocker input_blocker_;
    BasicRebindableInput input_{ window_, input_blocker_ };
    InputFreeCamera input_freecam_{ cam_ };

    RenderEngine rengine_{ registry_, cam_, globals::window_size.size_ref() };
    ImGuiContextWrapper imgui_{ window_ };
    ImGuiStageHooks imgui_stage_hooks_;
    ImGuiRegistryHooks imgui_registry_hooks_{ registry_ };

public:
    BoxScene3(glfw::Window& window)
        : window_{ window }
    {
        input_freecam_.configure(input_);

        input_.set_keybind(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
            if (args.is_released()) {
                imgui_stage_hooks_.hidden ^= true;
                imgui_registry_hooks_.hidden ^= true;
            }
        });

        window_.framebufferSizeEvent.setCallback(
            [this](glfw::Window& window, int w, int h) {
                using namespace gl;
                globals::window_size.set_to(w, h);
                glViewport(0, 0, w, h);
                rengine_.reset_size(w, h);
            }
        );
        rengine_.stages().emplace_back(AmbientBackgroundStage());
        rengine_.stages().emplace_back(MaterialDSMultilightStage());
        rengine_.stages().emplace_back(PointLightSourceBoxStage());

        imgui_stage_hooks_.add_hook("Point Light Boxes",
            PointLightSourceBoxStageImGuiHook(
                *rengine_.stages().back().target<PointLightSourceBoxStage>()
            )
        );

        imgui_registry_hooks_.add_hook("Lights", ImGuiRegistryLightComponentsHook());
        imgui_registry_hooks_.add_hook("Models", ImGuiRegistryModelComponentsHook());


        init_registry();
    }

    void process_input() {}

    void update() {
        input_freecam_.update();
    }

    void render() {
        imgui_.new_frame();

        rengine_.render();

        imgui_registry_hooks_.display();
        imgui_stage_hooks_.display();

        imgui_.render();
        update_input_blocker_from_imgui_io_state();
    }

private:
    static const std::array<glm::vec3, 10> initial_box_positions;
    static const std::array<glm::vec3, 5> initial_point_light_positions;

    void init_registry() {
        auto& r = registry_;

        // Add boxes
        Shared<Model> box = std::make_shared<Model>(
            AssimpModelLoader<>()
                .load("data/models/container/container.obj")
                .get()
        );

        for (size_t i{ 0 }; i < initial_box_positions.size(); ++i) {
            auto e = r.create();
            r.emplace<Transform>(e, Transform()
                .translate(initial_box_positions[i])
                .rotate(glm::radians(20.f * float(i)), { 1.0f, 0.3f, 0.5f })
            );
            r.emplace<Shared<Model>>(e, box);
        }

        // Add lights: ambi, dir, point
        r.emplace<light::Ambient>(r.create(), light::Ambient{
            .color = { 0.15f, 0.15f, 0.1f }
        });

        r.emplace<light::Directional>(r.create(), light::Directional{
            .color = { 0.3f, 0.3f, 0.2f },
            .direction = { -0.2f, -1.0f, -0.3f }
        });

        for (const auto& pos : initial_point_light_positions) {
            r.emplace<light::Point>(r.create(), light::Point{
                .color = glm::vec3(1.0f, 1.f, 0.8f),
                .position = pos,
                .attenuation = light::Attenuation{
                    .constant = 1.f,
                    .linear = 0.4f,
                    .quadratic = 0.2f
                }
            });
        }

    }

    void update_input_blocker_from_imgui_io_state() {
        // FIXME: Need a way to stop the ImGui window from recieving
        // mouse events when I'm in free cam.
        input_blocker_.block_keys = ImGui::GetIO().WantCaptureKeyboard;
        input_blocker_.block_scroll =
            ImGui::GetIO().WantCaptureMouse &&
            input_freecam_.state().is_cursor_mode;
    }

};


inline const std::array<glm::vec3, 10> BoxScene3::initial_box_positions {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

inline const std::array<glm::vec3, 5> BoxScene3::initial_point_light_positions{
    glm::vec3( 0.7f,  0.2f,  2.0f),
    glm::vec3( 2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3( 0.0f,  0.0f, -3.0f),
    glm::vec3( 0.0f,  1.0f,  0.0f)
};






} // namespace leakslearn

using leakslearn::BoxScene3;
