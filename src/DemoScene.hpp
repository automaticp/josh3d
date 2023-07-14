#pragma once
#include "AssimpModelLoader.hpp"
#include "Camera.hpp"
#include "CubemapData.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "ImGuiWindowSettings.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "MaterialDSN.hpp"
#include "Model.hpp"
#include "RenderComponents.hpp"
#include "RenderEngine.hpp"
#include "Shared.hpp"
#include "VertexPNT.hpp"
#include "hooks/DeferredShadingStageHook.hpp"
#include "hooks/GBufferStageHook.hpp"
#include "hooks/LightComponentsRegistryHook.hpp"
#include "hooks/ModelComponentsRegistryHook.hpp"
#include "hooks/PointLightSourceBoxStageHook.hpp"
#include "hooks/PostprocessBloomStageHook.hpp"
#include "hooks/PostprocessGammaCorrectionStageHook.hpp"
#include "hooks/PostprocessHDREyeAdaptationStageHook.hpp"
#include "hooks/ShadowMappingStageHook.hpp"
#include "stages/DeferredGeometryAnyMaterialStage.hpp"
#include "stages/DeferredGeometryStage.hpp"
#include "stages/DeferredShadingStage.hpp"
#include "stages/GBufferStage.hpp"
#include "stages/PointLightSourceBoxStage.hpp"
#include "stages/PostprocessBloomStage.hpp"
#include "stages/PostprocessGammaCorrectionStage.hpp"
#include "stages/PostprocessHDREyeAdaptationStage.hpp"
#include "stages/ShadowMappingStage.hpp"
#include "stages/SkyboxStage.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>
#include <imgui.h>


namespace leaksjosh {

using namespace josh;


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


        auto skyboxing    = rengine_.make_primary_stage<SkyboxStage>();

        auto shmapping    = rengine_.make_primary_stage<ShadowMappingStage>();
        auto gbuffer      = rengine_.make_primary_stage<GBufferStage>(w, h);

        // This is me sharing the depth target between the GBuffer and the
        // main framebuffer of the RenderEngine, so that deferred and forward draws
        // would overlap properly. Seems to work so far...
        auto gbuffer_write_handle = gbuffer.target().get_write_view();

        gbuffer_write_handle->attach_external_depth_buffer(rengine_.main_target().depth_target());

        auto defgeom     = rengine_.make_primary_stage<DeferredGeometryStage>(std::move(gbuffer_write_handle));
        auto defgeom_dsn =
            rengine_.make_primary_stage<DeferredGeometryAnyMaterialStage<MaterialDSN>>(
                gbuffer.target().get_write_view(),
                ShaderBuilder()
                    .load_vert("src/shaders/dfr_geometry_mat_dsn.vert")
                    .load_frag("src/shaders/dfr_geometry_mat_dsn.frag").get()
            );

        auto defshad =
            rengine_.make_primary_stage<DeferredShadingStage>(
                gbuffer.target().get_read_view(), shmapping.target().view_mapping_output()
            );

        auto plightboxes  = rengine_.make_primary_stage<PointLightSourceBoxStage>();

        auto blooming     = rengine_.make_postprocess_stage<PostprocessBloomStage>();
        auto hdreyeing    = rengine_.make_postprocess_stage<PostprocessHDREyeAdaptationStage>();
        auto whatsgamma   = rengine_.make_postprocess_stage<PostprocessGammaCorrectionStage>();


        imgui_stage_hooks_.add_hook("Shadow Mapping",     imguihooks::ShadowMappingStageHook(shmapping));
        imgui_stage_hooks_.add_hook("GBuffer",            imguihooks::GBufferStageHook(gbuffer));
        imgui_stage_hooks_.add_hook("Deferred Rendering", imguihooks::DeferredShadingStageHook(defshad));
        imgui_stage_hooks_.add_hook("Point Light Boxes",  imguihooks::PointLightSourceBoxStageHook(plightboxes));


        imgui_stage_hooks_.add_postprocess_hook("Bloom",
            imguihooks::PostprocessBloomStageHook(blooming));

        imgui_stage_hooks_.add_postprocess_hook("HDR Eye Adaptation",
            imguihooks::PostprocessHDREyeAdaptationStageHook(hdreyeing));

        imgui_stage_hooks_.add_postprocess_hook("Gamma Correction",
            imguihooks::PostprocessGammaCorrectionStageHook(whatsgamma));


        rengine_.add_next_primary_stage(std::move(skyboxing));
        rengine_.add_next_primary_stage(std::move(shmapping));
        rengine_.add_next_primary_stage(std::move(gbuffer));
        rengine_.add_next_primary_stage(std::move(defgeom));
        rengine_.add_next_primary_stage(std::move(defgeom_dsn));
        rengine_.add_next_primary_stage(std::move(defshad));
        rengine_.add_next_primary_stage(std::move(plightboxes));

        rengine_.add_next_postprocess_stage(std::move(blooming));
        rengine_.add_next_postprocess_stage(std::move(hdreyeing));
        rengine_.add_next_postprocess_stage(std::move(whatsgamma));



        imgui_registry_hooks_.add_hook("Lights", imguihooks::LightComponentsRegistryHook());
        imgui_registry_hooks_.add_hook("Models", imguihooks::ModelComponentsRegistryHook());

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


    constexpr const char* path = "data/models/shadow_scene/shadow_scene.obj";

    const auto model_entity = r.create();

    ModelComponentLoader()
        .load_into<VertexPNT, MaterialDS>(r, model_entity, path);

    r.emplace<Transform>(model_entity);
    r.emplace<components::Path>(model_entity, path);



    r.emplace<light::Ambient>(r.create(), light::Ambient{
        .color = { 0.15f, 0.15f, 0.1f }
    });

    auto e = r.create();
    r.emplace<light::Directional>(e, light::Directional{
        .color = { 0.15f, 0.15f, 0.1f },
        .direction = { -0.2f, -1.0f, -0.3f }
    });
    r.emplace<components::ShadowCasting>(e);

    components::Skybox skybox{ std::make_shared<Cubemap>() };
    skybox.cubemap->bind().attach_data(
        CubemapData::from_files(
            {
                "data/skyboxes/lake/right.png",
                "data/skyboxes/lake/left.png",
                "data/skyboxes/lake/top.png",
                "data/skyboxes/lake/bottom.png",
                "data/skyboxes/lake/front.png",
                "data/skyboxes/lake/back.png",
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




} // namespace leaksjosh

using leaksjosh::DemoScene;
