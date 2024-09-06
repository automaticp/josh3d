#pragma once
#include "AssetManager.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "Camera.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "Primitives.hpp"
#include "SceneImporter.hpp"
#include "SharedStorage.hpp"
#include "RenderEngine.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>


namespace josh { class GBuffer; }


class DemoScene {
public:
    DemoScene(glfw::Window& window);
    void applicate();
    void process_input();
    void update();
    void render();

private:
    glfw::Window&       window_;
    entt::registry      registry_;
    josh::AssetManager  assmanager_;
    josh::SceneImporter importer_;
    josh::Primitives    primitives_;
    josh::RenderEngine  rengine_;

    josh::SimpleInputBlocker   input_blocker_;
    josh::BasicRebindableInput input_;
    josh::InputFreeCamera      input_freecam_;

    josh::ImGuiApplicationAssembly imgui_;


    void configure_input(josh::SharedStorageView<josh::GBuffer> gbuffer);
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};

