#include "ImGuiVFSControl.hpp"
#include "Filesystem.hpp"
#include "VirtualFilesystem.hpp"
#include "VPath.hpp"
#include "ImGuiHelpers.hpp"
#include "RuntimeError.hpp"
#include <concepts>
#include <functional>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <type_traits>
#include <algorithm>
#include <utility>



namespace josh {


void ImGuiVFSControl::roots_listbox_widget() {

    using it_t = VFSRoots::const_iterator;
    VFSRoots& roots = vfs_.roots();

    auto to_remove = on_value_change_from(roots.end(),
        [&](const auto& to_remove) { roots.erase(to_remove); });


    if (ImGui::BeginListBox("VFS Roots")) {

        size_t imgui_id{ 0 };
        for (it_t it{ roots.cbegin() };
            it != roots.cend(); ++it, ++imgui_id)
        {
            ImGui::PushID(void_id(imgui_id));


            ImGui::BeginGroup();
            {
                ImGui::Selectable(it->path().c_str(), false,
                    ImGuiSelectableFlags_AllowItemOverlap);

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 2.f * ImGui::CalcTextSize("X").x);
                if (ImGui::SmallButton("X")) {
                    to_remove.set(it);
                }
            }
            ImGui::EndGroup();


            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("list_iterator", &it, sizeof(it));
                ImGui::Text("Move \"%s\" Before", it->path().c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("list_iterator"))
                {
                    auto drop_it = *static_cast<it_t*>(payload->Data);
                    roots.order_before(it, drop_it);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }


        // To enable dropping at the end.
        ImGui::Dummy(ImGui::GetItemRectSize());

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("list_iterator"))
            {
                auto drop_it = *static_cast<it_t*>(payload->Data);
                roots.order_before(roots.end(), drop_it);
            }
            ImGui::EndDragDropTarget();
        }


        ImGui::EndListBox();
    }

}




void ImGuiVFSControl::add_new_root_widget() {

    auto try_add_root = [&, this] {
        try {
            exception_str_ = "";
            vfs_.roots().push_front(Directory(new_root_));
        } catch (const error::RuntimeError& err) {
            exception_str_ = err.what();
        }
    };

    auto add_root_signal = on_value_change_from(false, try_add_root);

    if (ImGui::InputText("##New Root Input", &new_root_,
            ImGuiInputTextFlags_EnterReturnsTrue))
    {
        add_root_signal.set(true);
    }

    ImGui::SameLine();

    if (ImGui::Button("New Root##Button")) {
        add_root_signal.set(true);
    }

}




void ImGuiVFSControl::debug_resolve_widget() {


    if (ImGui::TreeNode("Debug")) {

        if (ImGui::Button("Clear Invalid Roots")) {
            size_t num_removed = vfs_.roots().remove_invalid();
            exception_str_ = "Removed " + std::to_string(num_removed) + " Invalid Roots";
        }


        auto try_resolve_file = [this] {
            exception_str_ = "";
            try {
                last_resolved_entry_ =
                    vfs_.resolve_file(VPath(test_vpath_)).path();
            } catch (const error::RuntimeError& err) {
                exception_str_ = err.what();
            }
        };

        auto try_resolve_dir = [this] {
            exception_str_ = "";
            try {
                last_resolved_entry_ =
                    vfs_.resolve_directory(VPath(test_vpath_)).path();
            } catch (const error::RuntimeError& err) {
                exception_str_ = err.what();
            }
        };


        if (ImGui::InputText("VPath", &test_vpath_)) {
            last_resolved_entry_ = "";
        }

        if (ImGui::Button("Resolve File")) { try_resolve_file(); }
        ImGui::SameLine();
        if (ImGui::Button("Resolve Directory")) { try_resolve_dir(); }


        ImGui::TextUnformatted(last_resolved_entry_.c_str());


        ImGui::TreePop();
    }


}




void ImGuiVFSControl::display() {

    if (hidden) { return; }

    ImGui::SetNextWindowSize({ 400.f, 400.f }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 600.f, 20.f }, ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);

    if (ImGui::Begin("VFS")) {

        roots_listbox_widget();
        add_new_root_widget();
        debug_resolve_widget();

        ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f }, "%s", exception_str_.c_str());

    } ImGui::End();
}




} // namespace josh
