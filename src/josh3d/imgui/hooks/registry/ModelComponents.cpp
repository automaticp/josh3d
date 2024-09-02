#include "ModelComponents.hpp"
#include "AssetManager.hpp"
#include "ComponentLoaders.hpp"
#include "ObjectLifecycle.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "RuntimeError.hpp"
#include "Model.hpp"
#include "Name.hpp"
#include "Filesystem.hpp"
#include "VPath.hpp"
#include "tags/Culled.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/ext/vector_common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glbinding/gl/gl.h>
#include <glm/gtx/component_wise.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>



namespace josh::imguihooks::registry {


ModelComponents::ModelComponents(AssetManager& assman)
    : assman_{ assman }
{}


void ModelComponents::load_model_widget(entt::registry& registry) {

    // FIXME: This is a total mess for now.
    // The models will not finish loading if the window is closed btw. Fun.

    struct Request {
        entt::entity             entity;
        Path                     path;
        Future<SharedModelAsset> future;
    };

    thread_local std::vector<Request> current_requests{};


    auto try_load_model = [&] {

        last_load_error_message_ = {};
        entt::handle model_handle{ registry, registry.create() };

        try {

            Path path{ load_path_ };

            if (path.is_absolute()) {

                File file{ path };
                model_handle.emplace<Path>(std::filesystem::canonical(file.path()));
                current_requests.emplace_back(model_handle.entity(), path, assman_.load_model(AssetPath{ file.path(), {} }));

            } else /* is_relative */ {

                VPath vpath{ path };
                File file{ vpath };
                model_handle.emplace<VPath>(std::move(vpath));
                model_handle.emplace<Path>(std::filesystem::canonical(file.path()));

                current_requests.emplace_back(model_handle.entity(), path, assman_.load_model(AssetPath{ file.path(), {} }));

            }

            model_handle.emplace<Name>(path.filename());
            model_handle.emplace<Transform>();

        } catch (const error::RuntimeError& e) {
            model_handle.destroy();
            last_load_error_message_ = e.what();
        } catch (...) {
            model_handle.destroy();
            throw;
        }

    };


    for (auto it = current_requests.begin();
        it != current_requests.end();)
    {
        auto& current_request = *it;
        if (current_request.future.is_available()) {
            if (registry.valid(current_request.entity)) {

                entt::handle model_handle{ registry, current_request.entity };

                try {

                    SharedModelAsset asset = get_result(std::move(current_request.future));
                    emplace_model_asset_into(model_handle, std::move(asset));

                } catch (const error::RuntimeError& e) {

                    model_handle.destroy();
                    last_load_error_message_ = e.what();

                } catch (...) {

                    model_handle.destroy();
                    throw;

                }

            }
            it = current_requests.erase(it);
        } else {
            ++it;
        }
    }



    auto load_model_signal = on_value_change_from(false, try_load_model);

    if (ImGui::InputText("##Path or VPath", &load_path_,
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        load_model_signal.set(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        load_model_signal.set(true);
    }

    ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f }, "%s", last_load_error_message_.c_str());

    if (ImGui::TreeNode("CurrentlyLoading", "Currently Loading (%zu)", current_requests.size())) {
        for (const auto& current_request : current_requests) {
            ImGui::PushID(void_id(current_request.entity));
            ImGui::Text("[%d] %s", entt::to_entity(current_request.entity), current_request.path.c_str());
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

}




void ModelComponents::model_list_widget(entt::registry& registry) {

    auto to_remove = on_value_change_from<entt::handle>({}, &destroy_subtree);

    for (auto [e, transform, model_component]
        : registry.view<Transform, Model>().each())
    {
        entt::handle model_handle{ registry, e };

        ImGui::PushID(void_id(e));

        const bool display_node = ImGui::TreeNodeEx(
            void_id(e),
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap,
            "%s", ""
        );

        ImGui::SameLine();
        if (imgui::ModelWidgetHeader(model_handle) == imgui::Feedback::Remove) {
            to_remove.set(model_handle);
        }

        if (display_node) {
            imgui::ModelWidgetBody(model_handle);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

}




void ModelComponents::operator()(
    entt::registry& registry)
{
    load_model_widget(registry);
    ImGui::Separator();
    model_list_widget(registry);
}


} // namespace josh::imguihooks::registry
