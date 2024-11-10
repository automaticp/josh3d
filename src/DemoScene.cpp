#include "DemoScene.hpp"
#include "Active.hpp"
#include "AssetManager.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "Logging.hpp"
#include "Camera.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "Primitives.hpp"
#include "SceneGraph.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "WindowSizeCache.hpp"
#include "FrameTimer.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"
#include "stages/overlay/SceneOverlays.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include "Filesystem.hpp"
#include "Tags.hpp"

#include "stages/precompute/PointLightSetup.hpp"
#include "stages/precompute/TransformResolution.hpp"
#include "stages/precompute/BoundingVolumeResolution.hpp"
#include "stages/precompute/FrustumCulling.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/SSAO.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "stages/primary/DeferredGeometry.hpp"
#include "stages/primary/TerrainGeometry.hpp"
#include "stages/primary/DeferredShading.hpp"
#include "stages/primary/LightDummies.hpp"
#include "stages/primary/Sky.hpp"
#include "stages/postprocess/Bloom2.hpp"
#include "stages/postprocess/FXAA.hpp"
#include "stages/postprocess/HDREyeAdaptation.hpp"
#include "stages/postprocess/Fog.hpp"
#include "stages/overlay/CSMDebug.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "stages/overlay/GBufferDebug.hpp"

#include "hooks/precompute/AllPrecomputeHooks.hpp"
#include "hooks/primary/AllPrimaryHooks.hpp"
#include "hooks/postprocess/AllPostprocessHooks.hpp"
#include "hooks/overlay/AllOverlayHooks.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>
#include <exception>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>


using namespace josh;


bool DemoScene::is_done() const noexcept {
    return window_.shouldClose();
}


void DemoScene::execute_frame() {

    josh::globals::frame_timer.update();

    render();

    glfw::pollEvents();
    process_input();
    update();

    window_.swapBuffers();
}


void DemoScene::process_input() {
    if (const auto camera = get_active<Camera, Transform>(registry_)) {
        input_freecam_.update(
            globals::frame_timer.delta<float>(),
            camera.get<Camera>(),
            camera.get<Transform>()
        );
    }
}


void DemoScene::update() {
    update_input_blocker_from_imgui_io_state();

    importer_.retire_completed_requests();
    while (importer_.can_unpack_more()) {
        try {
            importer_.unpack_one_retired();
        } catch (const std::exception& e) {
            logstream() << "[ERROR UNPACKING ASSET]: " << e.what() << "\n";
        }
    }

    if (shader_pool().supports_hot_reload()) {
        shader_pool().hot_reload();
    }

}


void DemoScene::render() {
    imgui_.new_frame();
    rengine_.render();
    imgui_.display();
}




DemoScene::DemoScene(glfw::Window& window)
    : window_       { window                  }
    , assmanager_   { vfs(), window           }
    , importer_     { assmanager_, registry_  }
    , primitives_   { assmanager_             }
    , rengine_      {
        registry_,
        primitives_,
        globals::window_size.size_ref(),
        RenderEngine::HDRFormat::R11F_G11F_B10F,
        globals::frame_timer
    }
    , input_blocker_{                         }
    , input_        { window_, input_blocker_ }
    , input_freecam_{                         }
    , imgui_        { window_, rengine_, registry_, importer_, vfs() }
{


    auto plightsetup   = stages::precompute::PointLightSetup();
    auto tfresolution  = stages::precompute::TransformResolution();
    auto bvresolution  = stages::precompute::BoundingVolumeResolution();
    auto frustumculler = stages::precompute::FrustumCulling();


    auto psmapping   = stages::primary::PointShadowMapping();
    auto csmapping   = stages::primary::CascadedShadowMapping();
    auto idbuffer    = stages::primary::IDBufferStorage(rengine_.main_resolution);
    auto gbuffer     =
        stages::primary::GBufferStorage(
            rengine_.main_resolution,
            // This is me sharing the depth target between the GBuffer and the
            // main framebuffer of the RenderEngine, so that deferred and forward draws
            // would overlap properly. Seems to work so far...
            rengine_.share_main_depth_attachment(),
            idbuffer.share_write_view()
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
    auto lightdummies = stages::primary::LightDummies(
        rengine_.share_main_depth_attachment(),
        rengine_.share_main_color_attachment(),
        idbuffer.share_write_view()
    );
    auto sky         = stages::primary::Sky();


    auto fog       = stages::postprocess::Fog();
    auto bloom2    = stages::postprocess::Bloom2();
    auto hdreyeing = stages::postprocess::HDREyeAdaptation(rengine_.main_resolution);
    auto fxaaaaaaa = stages::postprocess::FXAA();


    auto gbugger     = stages::overlay::GBufferDebug(gbuffer.share_read_view());
    auto csmbugger   = stages::overlay::CSMDebug(
        gbuffer.share_read_view(),
        csmapping.share_output_view()
    );
    auto ssaobugger  = stages::overlay::SSAODebug(
        ssao.share_output_view()
    );
    auto sceneovers  = stages::overlay::SceneOverlays();

    // FIXME: We need this for later, but for the wrong reasons (mouse picking).
    auto gbuffer_read_view = gbuffer.share_read_view();



    imgui_.stage_hooks().add_hook(imguihooks::precompute::PointLightSetup());
    imgui_.stage_hooks().add_hook(imguihooks::primary::PointShadowMapping());
    imgui_.stage_hooks().add_hook(imguihooks::primary::CascadedShadowMapping());
    imgui_.stage_hooks().add_hook(imguihooks::primary::DeferredGeometry());
    imgui_.stage_hooks().add_hook(imguihooks::primary::SSAO());
    imgui_.stage_hooks().add_hook(imguihooks::primary::DeferredShading());
    imgui_.stage_hooks().add_hook(imguihooks::primary::LightDummies());
    imgui_.stage_hooks().add_hook(imguihooks::primary::Sky());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::Fog());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::Bloom2());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::HDREyeAdaptation());
    imgui_.stage_hooks().add_hook(imguihooks::postprocess::FXAA());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::GBufferDebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::CSMDebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::SSAODebug());
    imgui_.stage_hooks().add_hook(imguihooks::overlay::SceneOverlays());




    rengine_.add_next_precompute_stage("Point Light Setup",    std::move(plightsetup));   // NOLINT(performance-move-const-arg)
    rengine_.add_next_precompute_stage("Transfrom Resolution", std::move(tfresolution));  // NOLINT(performance-move-const-arg)
    rengine_.add_next_precompute_stage("BV Resolution",        std::move(bvresolution));  // NOLINT(performance-move-const-arg)
    rengine_.add_next_precompute_stage("Frustum Culling",      std::move(frustumculler)); // NOLINT(performance-move-const-arg)

    rengine_.add_next_primary_stage("Point Shadow Mapping",      std::move(psmapping));
    rengine_.add_next_primary_stage("Cascaded Shadow Mapping",   std::move(csmapping));
    rengine_.add_next_primary_stage("IDBuffer",                  std::move(idbuffer));
    rengine_.add_next_primary_stage("GBuffer",                   std::move(gbuffer));
    rengine_.add_next_primary_stage("Deferred Mesh Geometry",    std::move(defgeom));
    rengine_.add_next_primary_stage("Deferred Terrain Geometry", std::move(terraingeom));
    rengine_.add_next_primary_stage("SSAO",                      std::move(ssao));
    rengine_.add_next_primary_stage("Deferred Shading",          std::move(defshad));
    rengine_.add_next_primary_stage("Light Dummies",             std::move(lightdummies));
    rengine_.add_next_primary_stage("Sky",                       std::move(sky));

    rengine_.add_next_postprocess_stage("Fog",                std::move(fog));       // NOLINT(performance-move-const-arg)
    rengine_.add_next_postprocess_stage("Bloom2",             std::move(bloom2));
    rengine_.add_next_postprocess_stage("HDR Eye Adaptation", std::move(hdreyeing));
    rengine_.add_next_postprocess_stage("FXAA",               std::move(fxaaaaaaa)); // NOLINT(performance-move-const-arg)

    rengine_.add_next_overlay_stage("GBuffer Debug",  std::move(gbugger));
    rengine_.add_next_overlay_stage("CSM Debug",      std::move(csmbugger));
    rengine_.add_next_overlay_stage("SSAO Debug",     std::move(ssaobugger));
    rengine_.add_next_overlay_stage("Scene Overlays", std::move(sceneovers));


    configure_input(gbuffer_read_view);
    init_registry();
}




void DemoScene::configure_input(SharedStorageView<GBuffer> gbuffer) {

    input_freecam_.configure(input_);

    input_.bind_key(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            imgui_.toggle_hidden();
        }
    });

    input_.bind_mouse_button(glfw::MouseButton::Left,
        [gbuffer=std::move(gbuffer), this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                const bool select_exact = args.mods & glfw::ModifierKeyBit::Control;
                const bool toggle_mode  = args.mods & glfw::ModifierKeyBit::Shift;


                auto [x, y] = args.window.getCursorPos();
                auto [sz_x, sz_y] = args.window.getFramebufferSize();
                int ix = int(x);
                int iy = int(y);

                entt::id_type id{ entt::null };

                // FIXME: This should probably be handled somewhere else.


                gbuffer->object_id_texture().download_image_region_into(
                   // FIXME: Is this off-by-one?
                   { { ix, sz_y - iy }, { 1, 1 } },
                   PixelDataFormat::RedInteger, PixelDataType::UInt,
                   std::span{ &id, 1 }
                );


                entt::handle provoking_handle{ registry_, entt::entity{ id } };

                // Either the click intersected null value (background),
                // or something else has destroyed the entity after
                // the ID buffer was generated on the previous frame.
                // (Can this even happen? Either way, bail if so.)
                if (!provoking_handle.valid()) {
                    if (toggle_mode) {
                        // If we are in the toggle mode, then do nothing.
                    } else /* unique mode */ {
                        // Otherwise we probably want to deselect all current selections.
                        registry_.clear<Selected>();
                    }
                    return;
                }

                entt::handle target_handle = [&]() -> entt::handle {
                    if (select_exact) {
                        // Select mesh same id as returned.
                        return provoking_handle;
                    } else /* select root */ {
                        // Select the root of the tree if the mesh has parents.
                        // This will just return itself, if it has no parents.
                        return get_root_handle(provoking_handle);
                    }
                }();


                if (toggle_mode) {
                    // We add to current selection if not selected,
                    // and deselect if it was. Don't touch others.
                    switch_tag<Selected>(target_handle);
                } else /* unique mode */ {
                    // We deselect all others, and force select target.
                    registry_.clear<Selected>();
                    set_tag<Selected>(target_handle);
                }
            }
        }
    );

    input_.bind_mouse_button(glfw::MouseButton::Middle,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_operation()) {
                    using enum GizmoOperation;
                    case Translation: imgui_.active_gizmo_operation() = Rotation;    break;
                    case Rotation:    imgui_.active_gizmo_operation() = Scaling;     break;
                    case Scaling:     imgui_.active_gizmo_operation() = Translation; break;
                }
            }
        }
    );

    input_.bind_mouse_button(glfw::MouseButton::Right,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_space()) {
                    using enum GizmoSpace;
                    case World: imgui_.active_gizmo_space() = Local; break;
                    case Local: imgui_.active_gizmo_space() = World; break;
                }
            }
        }
    );


    window_.framebufferSizeEvent.setCallback(
        [this](glfw::Window& /* window */ , int w, int h) {
            globals::window_size.set_to({ w, h });
            rengine_.main_resolution = { w, h };
        }
    );

}




void DemoScene::init_registry() {
    auto& registry = registry_;

    const entt::handle alight_handle{ registry, registry.create() };
    alight_handle.emplace<AmbientLight>(AmbientLight{ .color = { 0.15f, 0.15f, 0.1f } });
    make_active<AmbientLight>(alight_handle);

    const entt::handle dlight_handle{ registry, registry.create() };
    const glm::quat dlight_orientation = glm::quatLookAt(glm::vec3{ -0.2f, -1.f, -0.3f }, { 0.f, 1.f, 0.f });
    dlight_handle.emplace<DirectionalLight>(DirectionalLight{ .color = { 0.15f, 0.15f, 0.1f } });
    dlight_handle.emplace<Transform>(Transform().rotate(dlight_orientation));
    set_tag<ShadowCasting>(dlight_handle);
    make_active<DirectionalLight>(dlight_handle);


    using namespace josh::filesystem_literals;

    const auto model_vpath  = "data/models/shadow_scene/shadow_scene.obj"_vpath;
    const AssetPath model_apath { File(model_vpath),  {} };

    importer_.request_model_import(model_apath);
    importer_.wait_until_all_pending_are_complete();
    importer_.retire_completed_requests();
    importer_.unpack_all_retired();


    const entt::handle camera_handle{ registry, registry.create() };
    const Camera::Params camera_params{
        .fovy_rad     = glm::radians(80.f),
        .aspect_ratio = globals::window_size.size_ref().aspect_ratio(),
        .z_near       = 0.1f,
        .z_far        = 500.f,
    };
    camera_handle.emplace<Camera>(camera_params);
    camera_handle.emplace<Transform>().translate({ 0.f, 1.f, 0.f });
    make_active<Camera>(camera_handle);

}




void DemoScene::update_input_blocker_from_imgui_io_state() {

    auto wants = imgui_.get_io_wants();
    input_blocker_.block_keys = wants.capture_keyboard;
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    input_blocker_.block_mouse_buttons = wants.capture_mouse;
    input_blocker_.block_scroll =
        wants.capture_mouse &&
        input_freecam_.state().is_cursor_mode;
}




