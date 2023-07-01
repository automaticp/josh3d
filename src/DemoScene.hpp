#pragma once
#include "AmbientBackgroundStage.hpp"
#include "AssimpModelLoader.hpp"
#include "CubemapData.hpp"
#include "DeferredGeometryStage.hpp"
#include "DeferredShadingStage.hpp"
#include "ForwardRenderingStage.hpp"
#include "GBufferStage.hpp"
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


class DemoScene {
private:
    glfw::Window& window_;

    entt::registry registry_;

    Camera cam_{ { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };

    SimpleInputBlocker   input_blocker_;
    BasicRebindableInput input_{ window_, input_blocker_ };
    InputFreeCamera      input_freecam_{ cam_ };

    RenderEngine rengine_{
        registry_, cam_,
        globals::window_size.size_ref(), globals::frame_timer
    };

    ImGuiContextWrapper imgui_{ window_ };
    ImGuiWindowSettings imgui_window_settings_{ window_ };
    ImGuiStageHooks     imgui_stage_hooks_;
    ImGuiRegistryHooks  imgui_registry_hooks_{ registry_ };

public:
    DemoScene(glfw::Window& window)
        : window_{ window }
    {
        configure_input();

        auto [w, h] = rengine_.window_size();


        // auto ambickground = rengine_.make_primary_stage<AmbientBackgroundStage>();
        auto skyboxing    = rengine_.make_primary_stage<SkyboxStage>();

        auto shmapping    = rengine_.make_primary_stage<ShadowMappingStage>();
        auto gbuffer      = rengine_.make_primary_stage<GBufferStage>(w, h);

        // This is me sharing the depth target between the GBuffer and the
        // main framebuffer of the RenderEngine, so that deferred and forward draws
        // would overlap properly. Seems to work so far...
        auto gbuffer_write_handle = gbuffer.target().get_write_view();

        gbuffer_write_handle->attach_external_depth_buffer(rengine_.main_target().depth_target());

        auto defgeom = rengine_.make_primary_stage<DeferredGeometryStage>(std::move(gbuffer_write_handle));


        auto defshad =
            rengine_.make_primary_stage<DeferredShadingStage>(
                gbuffer.target().get_read_view(), shmapping.target().view_mapping_output()
            );

        // auto frendering   = rengine_.make_primary_stage<ForwardRenderingStage>(shmapping.target().view_mapping_output());
        auto plightboxes  = rengine_.make_primary_stage<PointLightSourceBoxStage>();

        auto blooming     = rengine_.make_postprocess_stage<PostprocessBloomStage>();
        auto hdreyeing    = rengine_.make_postprocess_stage<PostprocessHDREyeAdaptationStage>();
        auto whatsgamma   = rengine_.make_postprocess_stage<PostprocessGammaCorrectionStage>();


        imgui_stage_hooks_.add_hook("Shadow Mapping",    ShadowMappingStageImGuiHook(shmapping));
        // imgui_stage_hooks_.add_hook("Forward Rendering", ForwardRenderingStageImGuiHook(frendering));
        imgui_stage_hooks_.add_hook("GBuffer", GBufferStageImGuiHook(gbuffer));
        imgui_stage_hooks_.add_hook("Deferred Rendering", DeferredShadingStageImGuiHook(defshad));
        imgui_stage_hooks_.add_hook("Point Light Boxes", PointLightSourceBoxStageImGuiHook(plightboxes));


        imgui_stage_hooks_.add_postprocess_hook("Bloom",
            PostprocessBloomStageImGuiHook(blooming));

        imgui_stage_hooks_.add_postprocess_hook("HDR Eye Adaptation",
            PostprocessHDREyeAdaptationStageImGuiHook(hdreyeing));

        imgui_stage_hooks_.add_postprocess_hook("Gamma Correction",
            PostprocessGammaCorrectionStageImGuiHook(whatsgamma));


        // rengine_.add_next_primary_stage(std::move(ambickground));
        rengine_.add_next_primary_stage(std::move(skyboxing));
        rengine_.add_next_primary_stage(std::move(shmapping));
        rengine_.add_next_primary_stage(std::move(gbuffer));
        rengine_.add_next_primary_stage(std::move(defgeom));
        rengine_.add_next_primary_stage(std::move(defshad));
        // rengine_.add_next_primary_stage(std::move(frendering));
        rengine_.add_next_primary_stage(std::move(plightboxes));

        rengine_.add_next_postprocess_stage(std::move(blooming));
        rengine_.add_next_postprocess_stage(std::move(hdreyeing));
        rengine_.add_next_postprocess_stage(std::move(whatsgamma));



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

        imgui_window_settings_.display();
        imgui_registry_hooks_.display();
        imgui_stage_hooks_.display();

        imgui_.render();

        update_input_blocker_from_imgui_io_state();
    }

private:
    void configure_input();
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};





inline void DemoScene::configure_input() {

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

}




inline void DemoScene::init_registry() {
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




inline void DemoScene::update_input_blocker_from_imgui_io_state() {
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    input_blocker_.block_keys = ImGui::GetIO().WantCaptureKeyboard;
    input_blocker_.block_scroll =
        ImGui::GetIO().WantCaptureMouse &&
        input_freecam_.state().is_cursor_mode;
}




} // namespace leakslearn

using leakslearn::DemoScene;
