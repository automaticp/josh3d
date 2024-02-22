#include "TransformResolution.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, TransformResolution) {

    using Strat = stages::precompute::TransformResolution::Strategy;

    const char* strategy_names[] = {
        "Branch on Children",
        "Models -> Branch on Children",
        "Models -> Children -> The Rest",
        "Top-Down Models -> The Rest"
    };

    int strat_id = to_underlying(stage_.strategy);
    if (ImGui::ListBox("Strategy", &strat_id,
        strategy_names, std::size(strategy_names), 4))
    {
        stage_.strategy = Strat{ strat_id };
    }

}
