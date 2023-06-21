#pragma once
#include "AmbientBackgroundStage.hpp"
#include "AssimpModelLoader.hpp"
#include "CubemapData.hpp"
#include "ForwardRenderingStage.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "ImGuiWindowSettings.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "PointLightSourceBoxStage.hpp"
#include "PostprocessBloomStage.hpp"
#include "PostprocessGammaCorrectionStage.hpp"
#include "PostprocessHDREyeAdaptationStage.hpp"
#include "RenderComponents.hpp"
#include "RenderEngine.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Camera.hpp"
#include "ShadowMappingStage.hpp"
#include "Shared.hpp"
#include "SkyboxStage.hpp"
#include "RenderComponents.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
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

    RenderEngine rengine_{
        registry_, cam_,
        globals::window_size.size_ref(), globals::frame_timer
    };
    ImGuiContextWrapper imgui_{ window_ };
    ImGuiWindowSettings imgui_window_settings_{ window_ };
    ImGuiStageHooks imgui_stage_hooks_;
    ImGuiRegistryHooks imgui_registry_hooks_{ registry_ };

public:
    DirPointShadowScene(glfw::Window& window)
        : window_{ window }
    {
        input_freecam_.configure(input_);

        input_.set_keybind(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
            if (args.is_released()) {
                imgui_window_settings_.hidden ^= true;
                imgui_stage_hooks_.hidden ^= true;
                imgui_registry_hooks_.hidden ^= true;
            }
        });

        window_.framebufferSizeEvent.setCallback(
            [this](glfw::Window& /* window */ , int w, int h) {
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
            .emplace_back(SkyboxStage());



        ShadowMappingStage shmapping{};
        auto output_view = shmapping.view_mapping_output();

        rengine_.stages()
            .emplace_back(std::move(shmapping));

        imgui_stage_hooks_.add_hook("Shadow Mapping",
            ShadowMappingStageImGuiHook(
                rengine_.stages().back()
                    .target_unchecked<ShadowMappingStage>()
            )
        );

        rengine_.stages()
            .emplace_back(ForwardRenderingStage(std::move(output_view)));

        imgui_stage_hooks_.add_hook("Forward Rendering",
            ForwardRenderingStageImGuiHook(
                rengine_.stages().back()
                    .target_unchecked<ForwardRenderingStage>()
            )
        );



        rengine_.stages()
            .emplace_back(PointLightSourceBoxStage());

        imgui_stage_hooks_.add_hook("Point Light Boxes",
            PointLightSourceBoxStageImGuiHook(
                rengine_.stages().back()
                    .target_unchecked<PointLightSourceBoxStage>()
            )
        );



        imgui_registry_hooks_.add_hook("Lights", ImGuiRegistryLightComponentsHook());
        imgui_registry_hooks_.add_hook("Models", ImGuiRegistryModelComponentsHook());



        rengine_.postprocess_stages()
            .emplace_back(PostprocessBloomStage());

        imgui_stage_hooks_.add_postprocess_hook("Bloom",
            PostprocessBloomStageImGuiHook(
                rengine_.postprocess_stages().back()
                    .target_unchecked<PostprocessBloomStage>()
            )
        );



        rengine_.postprocess_stages()
            .emplace_back(PostprocessHDREyeAdaptationStage());

        imgui_stage_hooks_.add_postprocess_hook(
            "HDR Eye Adaptation",
            PostprocessHDREyeAdaptationStageImGuiHook(
                rengine_.postprocess_stages().back()
                    .target_unchecked<PostprocessHDREyeAdaptationStage>()
            )
        );



        rengine_.postprocess_stages()
            .emplace_back(PostprocessGammaCorrectionStage());

        imgui_stage_hooks_.add_postprocess_hook("Gamma Correction",
            PostprocessGammaCorrectionStageImGuiHook(
                rengine_.postprocess_stages().back()
                    .target_unchecked<PostprocessGammaCorrectionStage>()
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

        imgui_window_settings_.display();
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
        r.emplace<components::ShadowCasting>(e);

        components::Skybox skybox{ std::make_shared<Cubemap>() };
        skybox.cubemap->bind().attach_data(
            CubemapData::from_files(
                {
                    "data/textures/skybox/lake/right.png",
                    "data/textures/skybox/lake/left.png",
                    "data/textures/skybox/lake/top.png",
                    "data/textures/skybox/lake/bottom.png",
                    "data/textures/skybox/lake/front.png",
                    "data/textures/skybox/lake/back.png",
                }
            ), gl::GL_SRGB_ALPHA
        );
        r.emplace<components::Skybox>(r.create(), std::move(skybox));

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
