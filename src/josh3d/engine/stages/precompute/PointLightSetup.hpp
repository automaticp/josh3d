#pragma once
#include "RenderEngine.hpp"
#include "LightCasters.hpp"
#include "BoundingSphere.hpp"
#include <glm/common.hpp>
#include <limits>


namespace josh::stages::precompute {


class PointLightSetup {
public:
    float threshold_fraction{ 0.005f };

    void operator()(RenderEnginePrecomputeInterface& engine);

};




inline void PointLightSetup::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    // TODO: max_attenuation should be chosen based on the light source intensity.
    const float max_attenuation = threshold_fraction;

    auto solve_for_distance = [](
        const Attenuation& attenuation_coeffs,
        float                     attenuation)
            -> float
    {
        /*
        Source light color Cs is attenuated by a factor A
        over distance r from the source point, and produces apparent color Ca:

            Ca(r) = A(r) * Cs

        Attenuation factor A is related to the attenuation coefficients a, b, c as:

           1/A = a*r^2 + b*r + c

        where a > 0, b and c >= 0.

        For a mathematical point source: b = 0 and c = 0, and the attenuation
        is described by a singular ~ r^-2 term.

        There are no point sources IRL however, as every light source has finite surface area.
        Coefficient b and c can be used to roughly approximate non-infinitesimal sources.

        Solving for r we get some bouts of high school (or middle school?) math:

            D = b^2 - 4*a*(c - 1/A)

            r = (-b +/- sqrt(D)) / (2*a)

        The solution only exists if D >= 0, that is, the following holds:

            (b^2 / 4*a) >= (c - 1/A)

        - If c < 1/A, this always holds true, since a > 0 and b >= 0.
        - If c = 1/A, this also always holds true, but both solutions
          (r = 0 and r = -b/a) are outside of the domain of r > 0.
        - If c > 1/A, the solution can exist if the above still holds,
          but it will be strictly at r < 0 (again, not useful to us).

        Thus we need to always satisfy c < 1/A by either redifining A
        or clamping the expression (c - 1/A) between -inf and -(something small).

        Due to floating point normalization it's probably better if we clamp r too.
        */

        constexpr float something_small = 0.0001f; // Yeah, well...
        constexpr float inf = std::numeric_limits<float>::infinity();


        const auto [c, b, a] = attenuation_coeffs;
        const float A        = attenuation;

        const float D = (b * b) - 4.f * a * glm::clamp(c - 1 / A, -inf, -something_small);
        const float r = (-b + glm::sqrt(D)) / (2*a);

        return glm::clamp(r, something_small, inf);
    };


    for (auto [e, plight] : registry.view<PointLight>().each()) {

        const float max_distance =
            solve_for_distance(plight.attenuation, max_attenuation);

        registry.emplace_or_replace<BoundingSphere>(e, max_distance);
    }
}


} // namespace josh::stages::precompute
