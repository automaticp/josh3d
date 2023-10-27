#pragma once
#include "UniqueFunction.hpp"
#include <string>
#include <utility>
#include <vector>




namespace josh {



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
public:
    class HooksContainer {
    private:
        friend ImGuiStageHooks;
        struct HookEntry {
            HookEntry(UniqueFunction<void()> hook, std::string name)
                : hook(std::move(hook)), name(std::move(name))
            {}

            UniqueFunction<void()> hook;
            std::string name;
        };

        // FIXME: Multimap with typeid as key?
        std::vector<HookEntry> primary_hook_entries_;
        std::vector<HookEntry> pp_hook_entries_;
        std::vector<HookEntry> overlay_hook_entries_;

    public:
        void add_primary_hook(std::string name, UniqueFunction<void()> stage_hook) {
            primary_hook_entries_.emplace_back(std::move(stage_hook), std::move(name));
        }

        void add_postprocess_hook(std::string name, UniqueFunction<void()> postprocess_hook) {
            pp_hook_entries_.emplace_back(std::move(postprocess_hook), std::move(name));
        }

        void add_overlay_hook(std::string name, UniqueFunction<void()> overlay_hook) {
            overlay_hook_entries_.emplace_back(std::move(overlay_hook), std::move(name));
        }
    };

private:
    HooksContainer hooks_container_;

public:
    bool hidden{ false };

    HooksContainer&       hooks() noexcept       { return hooks_container_; }
    const HooksContainer& hooks() const noexcept { return hooks_container_; }

    void display();
};





} // namespace josh
