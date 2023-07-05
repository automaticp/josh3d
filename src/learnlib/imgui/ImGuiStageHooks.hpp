#pragma once
#include "GlobalsUtil.hpp"
#include "UniqueFunction.hpp"
#include <string>
#include <utility>
#include <imgui.h>




namespace learn {



/*
A container for various ImGui code that can be injected into
a general Render Stages debug window.

[Render Stages]
  [Primary]
    [Stage Name 1]
      <Your hook here>
    [Stage Name 2]
      <Your hook here>
    ...
  [Postprocessing]
    [Stage 1]
      <Your hook here>
    ...
*/
class ImGuiStageHooks {
private:
    struct HookEntry {
        HookEntry(UniqueFunction<void()> hook, std::string name)
            : hook(std::move(hook)), name(std::move(name))
        {}

        UniqueFunction<void()> hook;
        std::string name;
    };


    // FIXME: Multimap with typeid as key?
    std::vector<HookEntry> hooks_;
    std::vector<HookEntry> pp_hooks_;
public:
    bool hidden{ false };

public:
    void add_hook(std::string name, UniqueFunction<void()> stage_hook) {
        hooks_.emplace_back(std::move(stage_hook), std::move(name));
    }

    void add_postprocess_hook(std::string name, UniqueFunction<void()> postprocess_hook) {
        pp_hooks_.emplace_back(std::move(postprocess_hook), std::move(name));
    }

    void display() {
        if (hidden) { return; }

        ImGui::SetNextWindowSize({ 600.f, 400.f }, ImGuiCond_Once);
        ImGui::SetNextWindowPos({ 0.f, 600.f }, ImGuiCond_Once);
        if (ImGui::Begin("Render Stages")) {
            ImGui::Text("FPS: %.1f", 1.f / globals::frame_timer.delta<float>());

            if (ImGui::CollapsingHeader("Primary")) {
                for (size_t i{ 0 }; i < hooks_.size(); ++i) {

                    ImGui::PushID(int(i));
                    if (ImGui::TreeNode(hooks_[i].name.c_str())) {

                        hooks_[i].hook();

                        ImGui::TreePop();
                    }
                    ImGui::PopID();

                }
            }

            if (ImGui::CollapsingHeader("Postprocessing")) {
                for (size_t i{ 0 }; i < pp_hooks_.size(); ++i) {

                    ImGui::PushID(int(i));
                    if (ImGui::TreeNode(pp_hooks_[i].name.c_str())) {

                        pp_hooks_[i].hook();

                        ImGui::TreePop();
                    }
                    ImGui::PopID();

                }
            }

        } ImGui::End();
    }





};





} // namespace learn
