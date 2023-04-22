#pragma once
#include "AmbientBackgroundStage.hpp"
#include "AssimpModelLoader.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "PointLightSourceBoxStage.hpp"
#include "RenderEngine.hpp"
#include "MaterialDSMultilightShadowStage.hpp"
#include "Input.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Camera.hpp"
#include "Shared.hpp"
#include <glfwpp/window.h>
#include <entt/entt.hpp>
#include <imgui.h>

namespace leakslearn {

using namespace learn;


class DirPointShadowScene {
private:
    glfw::Window& window_;

    entt::registry registry_;
    Camera cam_{ { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };
    RebindableInputFreeCamera input_{ window_, cam_ };

    RenderEngine rengine_{ registry_, cam_, globals::window_size.size_ref() };
    ImGuiContextWrapper imgui_{ window_ };
    ImGuiStageHooks imgui_stage_hooks_;
    ImGuiRegistryHooks imgui_registry_hooks_{ registry_ };

public:
    DirPointShadowScene(glfw::Window& window)
        : window_{ window }
    {
        input_.set_keybind(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
            if (args.is_released()) {
                imgui_stage_hooks_.hidden ^= true;
                imgui_registry_hooks_.hidden ^= true;
            }
        });

        input_.use();

        rengine_.stages()
            .emplace_back(AmbientBackgroundStage());

        rengine_.stages()
            .emplace_back(MaterialDSMultilightShadowStage());

        imgui_stage_hooks_.add_hook("Multilight Shadows",
            MaterialDSMultilightShadowStageImGuiHook(
                *rengine_.stages().back().target<MaterialDSMultilightShadowStage>()
            )
        );

        rengine_.stages()
            .emplace_back(PointLightSourceBoxStage());

        imgui_stage_hooks_.add_hook("Point Light Boxes",
            PointLightSourceBoxStageImGuiHook(
                *rengine_.stages().back().target<PointLightSourceBoxStage>()
            )
        );

        imgui_registry_hooks_.add_hook("Lights", ImGuiRegistryLightComponentsHook());
        imgui_registry_hooks_.add_hook("Models", ImGuiRegistryModelComponentsHook());

        init_registry();
    }

    void process_input() {
        const bool ignore =
            ImGui::GetIO().WantCaptureKeyboard;
        input_.process_input(ignore);
    }
    void update() {}
    void render() {
        using namespace gl;
        imgui_.new_frame();
        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rengine_.render();

        imgui_registry_hooks_.display();
        imgui_stage_hooks_.display();

        imgui_.render();
    }

private:
    void init_registry() {
        auto& r = registry_;

        auto loader = AssimpModelLoader<>();

        Shared<Model> model = std::make_shared<Model>(
            loader.load("data/models/shadow_scene/shadow_scene.obj").get()
        );

        auto e = r.create();
        r.emplace<Shared<Model>>(e, std::move(model));
        r.emplace<Transform>(e);

        r.emplace<light::Ambient>(r.create(), light::Ambient{
            .color = { 0.15f, 0.15f, 0.1f }
        });

        r.emplace<light::Directional>(r.create(), light::Directional{
            .color = { 0.15f, 0.15f, 0.1f },
            .direction = { -0.2f, -1.0f, -0.3f }
        });


    }

};


};

using leakslearn::DirPointShadowScene;
