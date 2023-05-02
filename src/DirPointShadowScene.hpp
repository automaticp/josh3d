#pragma once
#include "AmbientBackgroundStage.hpp"
#include "AssimpModelLoader.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "PointLightSourceBoxStage.hpp"
#include "PostprocessBloomStage.hpp"
#include "PostprocessGammaCorrectionStage.hpp"
#include "PostprocessHDRStage.hpp"
#include "RenderEngine.hpp"
#include "MaterialDSMultilightShadowStage.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
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

    Camera cam_{ { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };
    SimpleInputBlocker input_blocker_;
    BasicRebindableInput input_{ window_, input_blocker_ };
    InputFreeCamera input_freecam_{ cam_ };

    RenderEngine rengine_{ registry_, cam_, globals::window_size.size_ref() };
    ImGuiContextWrapper imgui_{ window_ };
    ImGuiStageHooks imgui_stage_hooks_;
    ImGuiRegistryHooks imgui_registry_hooks_{ registry_ };

public:
    DirPointShadowScene(glfw::Window& window)
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
                // or
                // rengine_.reset_size_from_window_size();
            }
        );

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


        rengine_.postprocess_stages()
            .emplace_back(PostprocessBloomStage());

        imgui_stage_hooks_.add_postprocess_hook("Bloom",
            PostprocessBloomStageImGuiHook(
                *rengine_.postprocess_stages().back()
                    .target<PostprocessBloomStage>()
            )
        );

        rengine_.postprocess_stages()
            .emplace_back(PostprocessHDRStage());

        imgui_stage_hooks_.add_postprocess_hook("HDR",
            PostprocessHDRStageImGuiHook(
                *rengine_.postprocess_stages().back()
                    .target<PostprocessHDRStage>()
            )
        );

        rengine_.postprocess_stages()
            .emplace_back(PostprocessGammaCorrectionStage());

        imgui_stage_hooks_.add_postprocess_hook("Gamma Correction",
            PostprocessGammaCorrectionStageImGuiHook(
                *rengine_.postprocess_stages().back().target<PostprocessGammaCorrectionStage>()
            )
        );


        init_registry();
    }

    void process_input() {}

    void update() {
        input_freecam_.update();
    }
    void render() {
        using namespace gl;
        imgui_.new_frame();
        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rengine_.render();

        imgui_registry_hooks_.display();
        imgui_stage_hooks_.display();

        imgui_.render();
        update_input_blocker_from_imgui_io_state();
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

        e = r.create();
        r.emplace<light::Directional>(e, light::Directional{
            .color = { 0.15f, 0.15f, 0.1f },
            .direction = { -0.2f, -1.0f, -0.3f }
        });
        r.emplace<ShadowComponent>(e);

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


} // namespace leakslearn

using leakslearn::DirPointShadowScene;
