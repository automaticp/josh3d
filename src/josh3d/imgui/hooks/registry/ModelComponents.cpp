#include "ModelComponents.hpp"
#include "AssetManager.hpp"
#include "ComponentLoaders.hpp"
#include "GLMutability.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "RuntimeError.hpp"
#include "components/Materials.hpp"
#include "components/Model.hpp"
#include "components/Name.hpp"
#include "components/Path.hpp"
#include "components/VPath.hpp"
#include "tags/AlphaTested.hpp"
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
#include <optional>



namespace josh::imguihooks::registry {


ModelComponents::ModelComponents(AssetManager& assman)
    : assman_{ assman }
{}


void ModelComponents::load_model_widget(entt::registry& registry) {

    // FIXME: This is a total mess for now.

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
                model_handle.emplace<components::Path>(std::filesystem::canonical(file.path()));
                current_requests.emplace_back(model_handle.entity(), path, assman_.load_model(AssetPath{ file.path(), {} }));

            } else /* is_relative */ {

                VPath vpath{ path };
                File file{ vpath };
                model_handle.emplace<components::VPath>(std::move(vpath));
                model_handle.emplace<components::Path>(std::filesystem::canonical(file.path()));

                current_requests.emplace_back(model_handle.entity(), path, assman_.load_model(AssetPath{ file.path(), {} }));

            }

            model_handle.emplace<components::Name>(path.filename());
            model_handle.emplace<components::Transform>();

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

    if (ImGui::TreeNode("Show Currently Loading")) {
        for (const auto& current_request : current_requests) {
            ImGui::PushID(void_id(current_request.entity));
            ImGui::Text("[%d] %s", entt::to_entity(current_request.entity), current_request.path.c_str());
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

}




static void mesh_subwidget(entt::handle mesh) {

    const char* name = mesh.all_of<components::Name>() ?
        mesh.get<components::Name>().name.c_str() : "(No Name)";

    const char* culled_cstr =
        mesh.all_of<tags::Culled>() ? "(Culled)" : "";

    if (ImGui::TreeNode(void_id(mesh.entity()), "Mesh [%d]%s: %s",
        entt::to_entity(mesh.entity()), culled_cstr, name))
    {

        imgui::TransformWidget(&mesh.get<Transform>());

        bool is_alpha_tested = mesh.all_of<tags::AlphaTested>();
        if (ImGui::Checkbox("Alpha-Testing", &is_alpha_tested)) {
            if (is_alpha_tested) {
                mesh.emplace<tags::AlphaTested>();
            } else {
                mesh.remove<tags::AlphaTested>();
            }
        }

        if (ImGui::TreeNode("Material")) {

            // FIXME: Not sure if scaling to max size is always preferrable.
            auto imsize = [&](RawTexture2D<GLConst> tex) -> ImVec2 {
                const float w = ImGui::GetContentRegionAvail().x;
                const float h = w / tex.get_resolution().aspect_ratio();
                return { w, h };
            };

            if (auto material = mesh.try_get<components::MaterialDiffuse>()) {
                if (ImGui::TreeNode("Diffuse")) {
                    ImGui::Unindent();

                    imgui::ImageGL(void_id(material->texture->id()), imsize(material->texture));

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            if (auto material = mesh.try_get<components::MaterialSpecular>()) {
                if (ImGui::TreeNode("Specular")) {
                    ImGui::Unindent();

                    imgui::ImageGL(void_id(material->texture->id()), imsize(material->texture));

                    ImGui::DragFloat(
                        "Shininess", &material->shininess,
                        1.0f, 0.1f, 1.e4f, "%.3f", ImGuiSliderFlags_Logarithmic
                    );

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            if (auto material = mesh.try_get<components::MaterialNormal>()) {
                if (ImGui::TreeNode("Normal")) {
                    ImGui::Unindent();

                    imgui::ImageGL(void_id(material->texture->id()), imsize(material->texture));

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

}




void ModelComponents::model_list_widget(entt::registry& registry) {

    auto to_remove = on_value_change_from<entt::entity>(
        entt::null,
        [&](const entt::entity& model_ent) {
            auto& model = registry.get<components::Model>(model_ent);
            registry.destroy(model.meshes().begin(), model.meshes().end());
            registry.destroy(model_ent);
        }
    );

    for (auto [e, transform, model_component]
        : registry.view<Transform, components::Model>().each())
    {
        components::Path* path = registry.try_get<components::Path>(e);
        const char* path_cstr = path ? path->c_str() : "(No Path)";
        components::Name* name = registry.try_get<components::Name>(e);
        const char* name_cstr = name ? name->name.c_str() : "(No Name)";

        ImGui::PushID(void_id(e));

        const bool display_node =
            ImGui::TreeNode(void_id(e), "Model [%d]: %s",
                entt::to_entity(e), name_cstr);


        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            to_remove.set(e);
        }

        if (display_node) {
            ImGui::TextUnformatted(path_cstr);

            imgui::TransformWidget(&transform);

            for (auto mesh_entity : model_component.meshes()) {    ;
                mesh_subwidget({ registry, mesh_entity });
            }

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
