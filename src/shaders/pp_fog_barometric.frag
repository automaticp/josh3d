#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"




in  vec2 tex_coords;
out vec4 frag_color;


uniform float scale_height;
uniform float density_at_eye_height;
uniform vec3  world_up_in_view_space;


// Exponential visibility (direct transmittance) that simulates fog,
// with density decaying exponentially with height.
// Physical, not exactly fog-like, but has a relatively simple analytic solution.
float transmittance(vec3 frag_pos_view, float screen_depth) {
    /*
    See J. Legakis, "Fast Multi-Layer Fog":
        https://dl.acm.org/doi/pdf/10.1145/280953.282233
    And V. Biri, S. Michelin and D. Arquès "Real-Time Animation of Realistic Fog":
        https://hal.science/hal-00622212/file/biri_v_elversion2.pdf
    For a more in-depth exploration of this.

    Basic vocabulary:
        Re is the eye/camera position in world space;
        Rf is the fragment position in world space;
        {X|Y|Z}e and {X|Y|Z}f are cartesian components of those positions.
        dR = length(Re - Rf) and corresponding d{X|Y|Z} = {X|Y|Z}e - {X|Y|Z}f
            are distances between the eye and the fragment;
        dD = length(Re.xz - Rf.xz) is a horizontal distance between eye and fragment.


    The basic transmittance for a fog with variable density
    along the Y-axis is then given by (see Legakis):
        tau(Re, Rf) = exp(-k * I)                                       (1)
    where:
        k = dR / abs(dY) = sqrt(1 + (dD/dY)^2)
        I = integral from (Ye) to (Yf) of (rho(y) * dy)

    If the fog is assumed to be a simple isothermal ideal gas in a constant gravity field,
    then it's vertical density can be described by:
        rho(y) = rho(0) * exp(-y/H)                                     (2)
    where rho(0) is just a fog density at height 0 and H (scale height)
    describes how quickly the density of the fog falls off with height
    (density decreases by a factor of e over vertical distance H).

    Taking the integral I then gives:
        I = rho(0) * H * (exp(-Ye/H) - exp(-Yf/H))                      (3)


    We also have to consider a special case where dY is 0 and form (1) becomes
    inadequate due to division by 0 in k and I collapsing to 0 (indeterminate as 0/0).
    In such case, the transmittence is expressed more directly as:
        tau(Re, Rf) = exp(-dD * rho(Ye))    (only if Ye == Yf)          (4)


    So far we have an analytic transmittence model with 2 free parameters
    that describe the fog:
        H      - scale height
        rho(0) - density at point y = 0

    However, it is more convinient to describe the fog in the scene
    with 3 spatial parameters instead:
        H      - scale height
        Y0     - base height of the fog (some scene-specific ground level)
        L0     - horizontal mean free path at base height Y0

    Which means that at exactly Y0 the transmittance is equal to
        tau = exp(-dD / L0)
    at the same time, since we're looking at a horizontal slice: Ye == Yf == Y0,
    and we can evaluate form (4) at Y0 to obtain
        tau = exp(-dD * rho(Y0))

    Combining this and expression (2) we get:
        1 / L0 = rho(Y0) = rho(0) * exp(-Y0/H)
    which allows us to express rho(0) in terms of H, L0 and Y0 as:
        rho(0) = exp(Y0/H) / L0                                       (5)

    This can be computed on the CPU ahead of time.


    Another issue arises because we specified our equations in absolute
    world-space coordinates. This inadvertently leads to precision issues
    at large distances away from the world origin. We can, however, respecify
    the origin for computations to be in eye-space instead.

    According to (2), the density at eye height would be given by:
        rho(Ye) = rho(0) * exp(-Ye / H)                               (6)
    With this we can transform the expression (3) for I into view-space.
    Simple substitution of (6) into (3) gives:
        I = H * (rho(Ye) - rho(0) * exp(-Yf/H)) = H * rho(Ye) * (1.0  - exp(Ye - Yf/H))
    Or, remembering dY as Ye - Yf:
        I = H * rho(Ye) * (1.0 - exp(dY/H))                           (7)

    Since dY is independent of the eye position in world-space, so is I.
    The rho(Ye) can be precomputed ahead of time on the CPU with higher precision.
    The rest of the parameters already did not depend on absolute position, only on dY and dD.
    */
    const float H      = scale_height;
    const float rho_Ye = density_at_eye_height;

    // The projection with the dot() product has some precision loss
    // that we'll have to account for later, but not as bad as length().
    float dY = -dot(frag_pos_view, world_up_in_view_space);

    // The length() has a terrible precision loss because of sqaring,
    // and we can end up with (dR < abs(dY)) when dR ~ dY,
    // which makes no sense and produces NaNs further in calculations.
    // So we clamp dR to dY.
    float dR = max(length(frag_pos_view), abs(dY));

    float dD = sqrt((dR - dY) * (dR + dY));

    // This ratio needs to be preserved, as it represents the
    // angle of the inclination of the view-ray towards the horizon plane
    // (theta in world-space spherical coordinates).
    const float dD_over_dY = dD / dY;

    // We clamp to infinity the pixels that sit at the far plane.
    // Otherwise background fragments have ~z_far depth and are
    // only partially occluded by fog.
    const float far_plane_correction = (1.0 - step(1.0, screen_depth));
    dY /= far_plane_correction;
    dD /= far_plane_correction;

    // In world-space this would not be an issue, but
    // reprojection with the dot product introduces imprecision
    // and dY != 0.0 check is not sufficient.
    const bool is_in_horizontal_plane = abs(dY) < 0.001;

    if (isinf(dY) && (dY > 0.0)) {
        // If it's background and in the bottom hemisphere.
        return 0.0;
    } else if (!is_in_horizontal_plane) {

        const float k = sqrt(1.0 + (dD_over_dY * dD_over_dY));
        const float I = H * rho_Ye * (1.0 - exp(dY / H));
        // This still has an issue for distant meshes
        // where the rho_Ye -> 0 and exp(dY/H) -> inf
        // at the same time, so you get NaNs.

        return exp(sign(dY) * k * I);
    } else /* dY == 0.0 */ {
        return exp(-dD * rho_Ye);
    }

}


uniform sampler2D depth;

uniform vec3  fog_color;
uniform float z_near;
uniform float z_far;
uniform mat4  inv_proj;




void main() {

    float screen_depth = texture(depth, tex_coords).r;
    float view_depth   = get_view_space_depth(screen_depth, z_near, z_far);

    float w_clip   = view_depth;
    vec4  pos_ndc  = vec4(vec3(tex_coords, screen_depth) * 2.0 - 1.0, 1.0);
    vec4  pos_clip = pos_ndc * w_clip;
    vec4  pos_view = inv_proj * pos_clip;

    float fog_factor = 1.0 - transmittance(pos_view.xyz, screen_depth);

    // And then just blend.
    frag_color = vec4(fog_color, fog_factor);

    // For debug, fog only:
    // frag_color = vec4(fog_color * vec3(fog_factor), 1.0);
}
