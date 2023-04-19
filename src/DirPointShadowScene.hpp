#pragma once
#include "AssimpModelLoader.hpp"
#include "ImGuiStageHooks.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "PointLightSourceBoxStage.hpp"
#include "RenderEngine.hpp"
#include "MaterialDSMultilightShadowStage.hpp"
#include "Input.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Camera.hpp"
#include "RenderTargetDepthCubemap.hpp"
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
    ImGuiStageHooks imgui_hooks_;

public:
    DirPointShadowScene(glfw::Window& window)
        : window_{ window }
    {
        input_.use();

        rengine_.stages()
            .emplace_back(MaterialDSMultilightShadowStage());

        imgui_hooks_.add_hook(
            MaterialDSMultilightShadowStageImGuiHook(
                *rengine_.stages().back().target<MaterialDSMultilightShadowStage>()
            )
        );

        rengine_.stages()
            .emplace_back(PointLightSourceBoxStage());

        imgui_hooks_.add_hook(
            PointLightSourceBoxStageImGuiHook(
                *rengine_.stages().back().target<PointLightSourceBoxStage>()
            )
        );

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

        update_gui();
        imgui_hooks_.display();

        imgui_.render();
    }

private:
    void update_gui() {
        ImGui::Begin("Debug");

        for (auto [entity, plight] : registry_.view<light::Point>().each()) {
            ImGui::PushID(static_cast<int>(entity));
            ImGui::DragFloat3(
                "Point Light Position", glm::value_ptr(plight.position),
                0.1f, -30.f, 30.f
            );
            ImGui::PopID();
        }

        ImGui::End();
    }

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

        auto plight = r.create();
        r.emplace<light::Point>(plight, light::Point{
                .color = glm::vec3(1.0f, 1.f, 0.8f),
                .position = { 0.f, 1.f, 0.f },
                .attenuation = light::Attenuation{
                    .constant = 1.f,
                    .linear = 0.4f,
                    .quadratic = 0.2f
                }
            }
        );
        r.emplace<RenderTargetDepthCubemap>(plight, 4096, 4096);

    }

};


};

using leakslearn::DirPointShadowScene;
