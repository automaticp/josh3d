#include "ImGuiVFSControl.hpp"
#include "Filesystem.hpp"
#include "Scalars.hpp"
#include "VirtualFilesystem.hpp"
#include "VPath.hpp"
#include "ImGuiHelpers.hpp"
#include "RuntimeError.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>


namespace josh {


void ImGuiVFSControl::_roots_listbox_widget()
{
    using it_t = VFSRoots::const_iterator;
    VFSRoots& roots = vfs_.roots();

    auto remove_later = on_value_change_from(roots.end(),
        [&](const auto& to_remove) { roots.erase(to_remove); });

    if (ImGui::BeginListBox("VFS Roots"))
    {
        usize imgui_id = 0;
        for (it_t it = roots.cbegin();
            it != roots.cend();
            ++it, ++imgui_id)
        {
            ImGui::PushID(void_id(imgui_id));
            ImGui::BeginGroup();
            {
                ImGui::Selectable(it->path().c_str(), false, ImGuiSelectableFlags_AllowItemOverlap);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 2.f * ImGui::CalcTextSize("X").x);
                if (ImGui::SmallButton("X"))
                    remove_later.set(it);
            }
            ImGui::EndGroup();

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("list_iterator", &it, sizeof(it));
                ImGui::Text("Move \"%s\" Before", it->path().c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("list_iterator"))
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

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("list_iterator"))
            {
                auto drop_it = *static_cast<it_t*>(payload->Data);
                roots.order_before(roots.end(), drop_it);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::EndListBox();
    }
}

void ImGuiVFSControl::_add_new_root_widget()
{
    auto try_add_root = [&, this]
    {
        try
        {
            _exception_str = "";
            vfs_.roots().push_front(Directory(_new_root));
        }
        catch (const error::RuntimeError& err)
        {
            _exception_str = err.what();
        }
    };

    auto add_root_later = on_signal(try_add_root);

    if (ImGui::InputText("##New Root Input", &_new_root, ImGuiInputTextFlags_EnterReturnsTrue))
        add_root_later.set(true);

    ImGui::SameLine();

    if (ImGui::Button("New Root##Button"))
        add_root_later.set(true);
}

void ImGuiVFSControl::_debug_resolve_widget()
{
    if (ImGui::TreeNode("Debug"))
    {
        if (ImGui::Button("Clear Invalid Roots"))
        {
            usize num_removed = vfs_.roots().remove_invalid();
            _exception_str = "Removed " + std::to_string(num_removed) + " Invalid Roots";
        }

        auto try_resolve_file = [this]
        {
            _exception_str = "";
            try
            {
                _last_resolved_entry = vfs_.resolve_file(VPath(_test_vpath)).path();
            }
            catch (const error::RuntimeError& err)
            {
                _exception_str = err.what();
            }
        };

        auto try_resolve_dir = [this]
        {
            _exception_str = "";
            try
            {
                _last_resolved_entry = vfs_.resolve_directory(VPath(_test_vpath)).path();
            }
            catch (const error::RuntimeError& err)
            {
                _exception_str = err.what();
            }
        };

        if (ImGui::InputText("VPath", &_test_vpath))
            _last_resolved_entry = "";

        if (ImGui::Button("Resolve File"))
            try_resolve_file();

        ImGui::SameLine();

        if (ImGui::Button("Resolve Directory"))
            try_resolve_dir();

        ImGui::TextUnformatted(_last_resolved_entry.c_str());

        ImGui::TreePop();
    }
}

void ImGuiVFSControl::display()
{
    _roots_listbox_widget();
    _add_new_root_widget();
    _debug_resolve_widget();

    ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f }, "%s", _exception_str.c_str());
}


} // namespace josh
