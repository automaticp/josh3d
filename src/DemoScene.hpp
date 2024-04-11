#pragma once
#include "AssimpModelLoader.hpp"
#include "ComponentLoaders.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "PerspectiveCamera.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "SharedStorage.hpp"
#include "WindowSizeCache.hpp"
#include "FrameTimer.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"

#include "components/ChildMesh.hpp"
#include "hooks/overlay/CSMDebug.hpp"
#include "hooks/overlay/SSAODebug.hpp"
#include "hooks/precompute/CSMSetup.hpp"
#include "hooks/precompute/TransformResolution.hpp"
#include "hooks/primary/DeferredGeometry.hpp"
#include "hooks/primary/SSAO.hpp"
#include "stages/overlay/CSMDebug.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "stages/precompute/CSMSetup.hpp"
#include "stages/precompute/FrustumCulling.hpp"
#include "stages/precompute/TransformResolution.hpp"
#include "stages/primary/SSAO.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include "components/Path.hpp"
#include "components/Name.hpp"

#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "stages/primary/DeferredGeometry.hpp"
#include "stages/primary/TerrainGeometry.hpp"
#include "stages/primary/DeferredShading.hpp"
#include "stages/primary/PointLightBox.hpp"
#include "stages/primary/Sky.hpp"
#include "stages/postprocess/Bloom.hpp"
#include "stages/postprocess/FXAA.hpp"
#include "stages/postprocess/HDREyeAdaptation.hpp"
#include "stages/postprocess/Fog.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "stages/overlay/SelectedObjectHighlight.hpp"
#include "stages/overlay/BoundingSphereDebug.hpp"

#include "hooks/primary/CascadedShadowMapping.hpp"
#include "hooks/primary/PointShadowMapping.hpp"
#include "hooks/primary/DeferredShading.hpp"
#include "hooks/primary/PointLightBox.hpp"
#include "hooks/primary/Sky.hpp"
#include "hooks/postprocess/Bloom.hpp"
#include "hooks/postprocess/FXAA.hpp"
#include "hooks/postprocess/HDREyeAdaptation.hpp"
#include "hooks/postprocess/Fog.hpp"
#include "hooks/overlay/GBufferDebug.hpp"
#include "hooks/overlay/SelectedObjectHighlight.hpp"
#include "hooks/overlay/BoundingSphereDebug.hpp"
#include "hooks/registry/LightComponents.hpp"
#include "hooks/registry/ModelComponents.hpp"
#include "hooks/registry/TerrainComponents.hpp"
#include "hooks/registry/SkyboxComponents.hpp"
#include "hooks/registry/PerspectiveCamera.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>


namespace leaksjosh {

using namespace josh;


class DemoScene {
private:
    glfw::Window& window_;

    entt::registry registry_;

    PerspectiveCamera cam_{
        PerspectiveCameraParams{
            .fovy_rad = glm::radians(90.f),
            .aspect_ratio = globals::window_size.size_ref().aspect_ratio(),
            .z_near  = 0.1f,
            .z_far   = 100.f
        },
        Transform().translate({ 0.f, 1.f, 0.f })
    };

    SimpleInputBlocker   input_blocker_;
    BasicRebindableInput input_{ window_, input_blocker_ };
    InputFreeCamera      input_freecam_{ cam_ };

    RenderEngine rengine_{
        registry_, cam_,
        globals::window_size.size_ref(), globals::frame_timer
    };

    ImGuiApplicationAssembly imgui_{ window_, rengine_, registry_, cam_, vfs() };

public:
    DemoScene(glfw::Window& window);
    void applicate();
    void process_input() {}
    void update();
    void render();

private:
    void configure_input(SharedStorageView<GBuffer> gbuffer);
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};




inline DemoScene::DemoScene(glfw::Window& window)
    : window_{ window }
{


    auto csmbuilder    = stages::precompute::CSMSetup();
    auto frustumculler = stages::precompute::FrustumCulling();
    auto tfresolution  = stages::precompute::TransformResolution();


    auto psmapping   = stages::primary::PointShadowMapping();
    auto csmapping   = stages::primary::CascadedShadowMapping(csmbuilder.share_output_view());
    auto gbuffer     =
        stages::primary::GBufferStorage(
            rengine_.main_resolution(),
            // This is me sharing the depth target between the GBuffer and the
            // main framebuffer of the RenderEngine, so that deferred and forward draws
            // would overlap properly. Seems to work so far...
            rengine_.share_main_depth_attachment()
        );
    auto defgeom     = stages::primary::DeferredGeometry(gbuffer.share_write_view());
    auto terraingeom = stages::primary::TerrainGeometry(gbuffer.share_write_view());
    auto ssao        = stages::primary::SSAO(gbuffer.share_read_view());
    auto defshad     =
        stages::primary::DeferredShading(
            gbuffer.share_read_view(),
            psmapping.share_output_view(),
            csmapping.share_output_view(),
            ssao.share_output_view()
        );
    auto plightboxes = stages::primary::PointLightBox();
    auto sky         = stages::primary::Sky();


    auto fog       = stages::postprocess::Fog();
    auto blooming  = stages::postprocess::Bloom(rengine_.main_resolution());
    auto hdreyeing = stages::postprocess::HDREyeAdaptation(rengine_.main_resolution());
    auto fxaaaaaaa = stages::postprocess::FXAA();


    auto gbugger     = stages::overlay::GBufferDebug(gbuffer.share_read_view());
    auto csmbugger   = stages::overlay::CSMDebug(
        gbuffer.share_read_view(),
        csmbuilder.share_output_view(),
        csmapping.share_output_view()
    );
    auto ssaobugger  = stages::overlay::SSAODebug(
        ssao.share_output_view()
    );
    auto selected    = stages::overlay::SelectedObjectHighlight();
    auto cullspheres = stages::overlay::BoundingSphereDebug();


    // FIXME: We need this for later, but for the wrong reasons (mouse picking).
    auto gbuffer_read_view = gbuffer.share_read_view();




    imgui_.stage_hooks().add_hook(imguihooks::precompute::CSMSetup());
    imgui_.stage_hooks().add_hook(imguihooks::precompute::TransformResolution());
    imgui_.stage_hooks().add_hook(imguihooks::primary::PointShadowMapping());
    imgui_.stage_hooks().add_hook(imguihooks::primary::CascadedShadowMapping());
    imgui_.stage_hooks().add_hook(imguihooks::primary::DeferredGeometry());
    imgui_.stage_hooks().add_hook(imguihooks::primary::SSAO());
    imgui_.stage_hooks().add_hook(imguihooks::primary::DeferredShading());
    imgui_.stage_hooks().add_hook(imguihooks::primary::PointLightBox());
    imgui_.stage_hooks().add_hook(imguihooks::primary::Sky());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::Fog());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::Bloom());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::HDREyeAdaptation());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::FXAA());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::GBufferDebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::CSMDebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::SSAODebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::SelectedObjectHighlight());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::BoundingSphereDebug());




    rengine_.add_next_precompute_stage("CSM Setup",            std::move(csmbuilder));
    rengine_.add_next_precompute_stage("Frustum Culling",      std::move(frustumculler));
    rengine_.add_next_precompute_stage("Transfrom Resolution", std::move(tfresolution));

    rengine_.add_next_primary_stage("Point Shadow Mapping",      std::move(psmapping));
    rengine_.add_next_primary_stage("Cascaded Shadow Mapping",   std::move(csmapping));
    rengine_.add_next_primary_stage("GBuffer",                   std::move(gbuffer));
    rengine_.add_next_primary_stage("Deferred Mesh Geometry",    std::move(defgeom));
    rengine_.add_next_primary_stage("Deferred Terrain Geometry", std::move(terraingeom));
    rengine_.add_next_primary_stage("SSAO",                      std::move(ssao));
    rengine_.add_next_primary_stage("Deferred Shading",          std::move(defshad));
    rengine_.add_next_primary_stage("Point Light Boxes",         std::move(plightboxes));
    rengine_.add_next_primary_stage("Sky",                       std::move(sky));

    rengine_.add_next_postprocess_stage("Fog",                std::move(fog));
    rengine_.add_next_postprocess_stage("Bloom",              std::move(blooming));
    rengine_.add_next_postprocess_stage("HDR Eye Adaptation", std::move(hdreyeing));
    rengine_.add_next_postprocess_stage("FXAA",               std::move(fxaaaaaaa));

    rengine_.add_next_overlay_stage("GBuffer Debug",             std::move(gbugger));
    rengine_.add_next_overlay_stage("CSM Debug",                 std::move(csmbugger));
    rengine_.add_next_overlay_stage("SSAO Debug",                std::move(ssaobugger));
    rengine_.add_next_overlay_stage("Selected Object Highlight", std::move(selected));
    rengine_.add_next_overlay_stage("Bounding Spheres",          std::move(cullspheres));




    imgui_.registry_hooks().add_hook("Lights",  imguihooks::registry::LightComponents());
    imgui_.registry_hooks().add_hook("Models",  imguihooks::registry::ModelComponents());
    imgui_.registry_hooks().add_hook("Skybox",  imguihooks::registry::SkyboxComponents());
    imgui_.registry_hooks().add_hook("Terrain", imguihooks::registry::TerrainComponents());
    imgui_.registry_hooks().add_hook("Camera",  imguihooks::registry::PerspectiveCamera(cam_));


    configure_input(gbuffer_read_view);

    init_registry();
}




inline void DemoScene::applicate() {

    while (!window_.shouldClose()) {
        josh::globals::frame_timer.update();

        render();

        glfw::pollEvents();
        process_input();
        update();

        window_.swapBuffers();
    }

}




inline void DemoScene::update() {
    update_input_blocker_from_imgui_io_state();
    input_freecam_.update();
}




inline void DemoScene::render() {
    imgui_.new_frame();
    rengine_.render();
    imgui_.display();
}




inline void DemoScene::configure_input(SharedStorageView<GBuffer> gbuffer) {

    input_freecam_.configure(input_);

    input_.bind_key(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            imgui_.toggle_hidden();
        }
    });

    input_.bind_mouse_button(glfw::MouseButton::Left,
        [gbuffer=std::move(gbuffer), this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                auto [x, y] = args.window.getCursorPos();
                auto [sz_x, sz_y] = args.window.getFramebufferSize();
                int ix = int(x);
                int iy = int(y);

                entt::id_type id{ entt::null };

                // FIXME: This should probably be handled in a precompute stage.


                gbuffer->object_id_texture().download_image_region_into(
                   // FIXME: Is this off-by-one?
                   { { ix, sz_y - iy }, { 1, 1 } },
                   PixelDataFormat::RedInteger, PixelDataType::UInt,
                   std::span{ &id, 1 }
                );


                entt::handle provoking_handle{ registry_, entt::entity{ id } };

                entt::handle target_handle = [&]() -> entt::handle {
                    if (args.mods & glfw::ModifierKeyBit::Control) {
                        // Select mesh (same id as returned)
                        return provoking_handle;
                    } else {
                        // Select the whole model if it's a ChildMesh
                        if (auto as_child = provoking_handle.try_get<components::ChildMesh>()) {
                            return { registry_, as_child->parent };
                        }
                        // Fallback for everything else.
                        return provoking_handle;
                    }
                }();

                const bool had_selection =
                    target_handle.valid() && target_handle.any_of<tags::Selected>();

                // Deselect all selected.
                registry_.clear<tags::Selected>();

                // Uhh...
                if (!had_selection && target_handle.valid()) {
                    target_handle.emplace<tags::Selected>();
                }

            }
        }
    );

    input_.bind_mouse_button(glfw::MouseButton::Middle,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_operation()) {
                    using enum GizmoOperation;
                    case translation: imgui_.active_gizmo_operation() = rotation;    break;
                    case rotation:    imgui_.active_gizmo_operation() = scaling;     break;
                    case scaling:     imgui_.active_gizmo_operation() = translation; break;
                }
            }
        }
    );

    input_.bind_mouse_button(glfw::MouseButton::Right,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_space()) {
                    using enum GizmoSpace;
                    case world: imgui_.active_gizmo_space() = local; break;
                    case local: imgui_.active_gizmo_space() = world; break;
                }
            }
        }
    );


    window_.framebufferSizeEvent.setCallback(
        [this](glfw::Window& /* window */ , int w, int h) {
            globals::window_size.set_to({ w, h });
            rengine_.resize({ w, h });
        }
    );

}




inline void DemoScene::init_registry() {
    auto& r = registry_;

    using enum GLenum;

    constexpr const char* path = "data/models/shadow_scene/shadow_scene.obj";
    constexpr const char* name = "shadow_scene.obj";

    entt::handle model{ r, r.create() };

    ModelComponentLoader()
        .load_into(model, VPath(path));

    model.emplace<Transform>();
    model.emplace<components::Path>(path);
    model.emplace<components::Name>(name);

    r.emplace<light::Ambient>(r.create(), light::Ambient{
        .color = { 0.15f, 0.15f, 0.1f }
    });

    auto e = r.create();
    r.emplace<light::Directional>(e, light::Directional{
        .color = { 0.15f, 0.15f, 0.1f },
        .direction = { -0.2f, -1.0f, -0.3f }
    });
    r.emplace<tags::ShadowCasting>(e);

    load_skybox_into(entt::handle{ r, r.create() }, VPath("data/skyboxes/yokohama/skybox.json"));
}




inline void DemoScene::update_input_blocker_from_imgui_io_state() {

    auto wants = imgui_.get_io_wants();
    input_blocker_.block_keys = wants.capture_keyboard;
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    input_blocker_.block_mouse_buttons = wants.capture_mouse;
    input_blocker_.block_scroll =
        wants.capture_mouse &&
        input_freecam_.state().is_cursor_mode;
}




} // namespace leaksjosh


using leaksjosh::DemoScene;
