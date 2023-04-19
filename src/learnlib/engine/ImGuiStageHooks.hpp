#pragma once
#include "GlobalsUtil.hpp"
#include "UniqueFunction.hpp"
#include <utility>
#include <imgui.h>




namespace learn {



/*
A container for various ImGui code that can be injected into
a general Render Stages debug window.

[Render Stages]
  [Primary]
    [Stage 1]
      <Your hook here>
    [Stage 2]
      <Your hook here>
    ...
  [Postprocessing]
    [Stage 1]
      <Your hook here>
    ...
*/
class ImGuiStageHooks {
public:
    using hook_t = UniqueFunction<void()>;

private:
    // FIXME: Multimap with typeid as key?
    std::vector<hook_t> hooks_;

public:
    void add_hook(hook_t stage_hook) {
        hooks_.emplace_back(std::move(stage_hook));
    }

    void display() {

        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
        if (ImGui::Begin("Render Stages", nullptr)) {
            ImGui::Text("FPS: %.1f", 1.f / globals::frame_timer.delta<float>());

            if (ImGui::CollapsingHeader("Primary")) {
                for (size_t i{ 0 }; i < hooks_.size(); ++i) {

                    if (ImGui::TreeNode(reinterpret_cast<void*>(i), "Stage %zu", i)) {

                        hooks_[i]();

                        ImGui::TreePop();
                    }

                }
            }

            if (ImGui::CollapsingHeader("Postprocessing")) {
                ImGui::Text("Not Implemented!");
            }

        } ImGui::End();
    }





};





} // namespace learn
