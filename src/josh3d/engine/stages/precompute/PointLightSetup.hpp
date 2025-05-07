#pragma once
#include "RenderEngine.hpp"
#include "ECS.hpp"
#include "LightCasters.hpp"
#include "BoundingSphere.hpp"
#include <glm/common.hpp>


namespace josh::stages::precompute {


class PointLightSetup {
public:
    enum class Strategy {
        FixedRadius,
        RadiosityThreshold,
        // ReverseExposure
    };

    Strategy strategy{ Strategy::RadiosityThreshold };

    float bounding_radius    { 10.0f  };
    float radiosity_threshold{ 0.005f };

    void operator()(RenderEnginePrecomputeInterface& engine);

};




inline void PointLightSetup::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    /*
    For a perfect point light, spectral radiosity transmitted by a spherical
    shell with radius r is:

        J(r) = P / (4 * pi * r^2)

    where P is the spectral power of the point source.

    Since initially, we specify P as the primary parameter of the point lights,
    it's spectral irradiance can be obtained by "attenuating" the spectral power.

    In the shader, we'll have something like

        struct PointLight {
            ...
            vec3 color;
            ...
        };

    Where each component of `color` represents spectral power of that particular
    color band. For a computed `r` between the point light and the fragment,
    the final fragment irradiance is obtained without any extra parameters:

        frag_color += point_ligth.color / (4 * pi * r^2);

    It is useful, however, to ignore this contribution if it is sufficiently small.
    This allows us to "cull" the light source entirely from computation.

    For that, we can define a spectral radiosity threshold J0, below which
    the light contribution will be ignored. If we can compute or define
    the J0 for each point source in the scene, this would allow us to construct
    a bounding sphere with radius r0 for culling:

        r0 = sqrt(P / (4 * pi * J0))

    Defining J0 can be done in multiple ways:

        1. By absolute attenutation threshold A0 = A(r0), where A(r) is defined as:

            A(r) = 1 / (4 * pi * r^2)

        Then the bounding radius is simply:

            r0 = sqrt(1 / (4 * pi * A0))

        and attenuation threshold can be related back to the J0 as:

            A0 = J0 / P

        This is quick and dirty purely geometric estimation, and has
        one disasterous downside:
            - The r0 is the same for *all* point lights, independent of their power.

        2. By absolute J0. This gives an already specified radius:

            r0 = sqrt(P / (4 * pi * J0))

        except that P is *spectral* and is per rgb component, so it makes sense
        to clamp to the highest `Pmax = max(Pr, Pg, Pb)`, such that:

            r0 = sqrt(Pmax / (4 * pi * J0))

        This gives us the bounding radius that actually *scales* with the light
        power P. The major downsides of this approach:
            - J0 must be manually set and adjusted per lighting conditions;
            - Bright sources in dark environments can have their light cut-off too early;
            - Dim sources in bright environments can have their bounding volumes way too big,
              reducing effectivness of culling.

        3. The main issue with the previous approach is that it does not consider
        the illumination conditions of the scene, that is, whether the contribution
        below the threshold J0 could still visually impact the scene and by how much.

        This is where the reverse exposure technique comes in to pin the J0 at the
        bottom of the effective dynamic range.

        More on that later (when I implement it :^).

    */


    using glm::vec3;
    using std::max, std::sqrt;
    constexpr float four_pi = 4.f * glm::pi<float>();


    if (strategy == Strategy::FixedRadius) {

        for (auto [e, plight] : registry.view<PointLight>().each()) {
            const float r0 = bounding_radius;
            registry.emplace_or_replace<LocalBoundingSphere>(e, glm::vec3{}, r0);
        }

    } else if (strategy == Strategy::RadiosityThreshold) {

        for (auto [e, plight] : registry.view<PointLight>().each()) {
            const vec3  P    = plight.hdr_color();
            const float Pmax = max({ P.x, P.y, P.z });
            const float J0   = radiosity_threshold;
            const float r0   = sqrt(Pmax / (four_pi * J0));
            registry.emplace_or_replace<LocalBoundingSphere>(e, glm::vec3{}, r0);
        }

    }
}


} // namespace josh::stages::precompute
