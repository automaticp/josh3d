#pragma once
#include "AssetImporter.hpp"
#include "AssetUnpacker.hpp"
#include "AssetManager.hpp"
#include "CompletionContext.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "MeshRegistry.hpp"
#include "Primitives.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceUnpacker.hpp"
#include "SceneImporter.hpp"
#include "SharedStorage.hpp"
#include "RenderEngine.hpp"
#include <boost/iostreams/tee.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>
#include <ostream>


namespace josh { class GBuffer; }


class DemoScene {
public:
    DemoScene(glfw::Window& window);
    void execute_frame();
    bool is_done() const noexcept;

    void process_input();
    void update();
    void render();

    auto get_log_sink() -> std::ostream& { return imgui_.get_log_sink(); }

private:
    glfw::Window&           window_;
    entt::registry          registry_;
    josh::ThreadPool        loading_pool_;
    josh::OffscreenContext  offscreen_context_;
    josh::CompletionContext completion_context_;
    josh::MeshRegistry      mesh_registry_;

    josh::AssetManager      asset_manager_;
    josh::AssetUnpacker     asset_unpacker_;
    josh::SceneImporter     scene_importer_;

    josh::ResourceDatabase  resource_database_;
    josh::AssetImporter     asset_importer_;
    josh::ResourceRegistry  resource_registry_;
    josh::ResourceUnpacker  resource_unpacker_;

    josh::Primitives        primitives_;
    josh::RenderEngine      rengine_;

    josh::SimpleInputBlocker   input_blocker_;
    josh::BasicRebindableInput input_;
    josh::InputFreeCamera      input_freecam_;

    josh::ImGuiApplicationAssembly imgui_;

    void configure_input(josh::SharedStorageView<josh::GBuffer> gbuffer);
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};

