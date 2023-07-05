#pragma once
#include "UniqueFunction.hpp"
#include <string>
#include <utility>
#include <vector>




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

    void display();

};





} // namespace learn
