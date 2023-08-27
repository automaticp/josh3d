#version 430 core

in  vec2 tex_coords;
out vec4 frag_color;


uniform vec3  cam_pos;
uniform float base_density;
uniform float scale_height;


// Exponential visibility (direct transmittance) that simulates fog,
// with density decaying exponentially with height.
// Physical, not exactly fog-like, but has a relatively simple analytic solution.
float transmittance(vec3 frag_pos_world) {
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
    */
    const float H    = scale_height;
    const float rho0 = base_density;

    const vec3  Re = cam_pos;
    const vec3  Rf = frag_pos_world;

    const float Ye = Re.y;
    const float Yf = Rf.y;

    const float dY = Ye - Yf;
    const float dD = length(Re.xz - Rf.xz);

    if (dY != 0.0) {

        const float dD_over_dY = dD / dY;
        const float k          = sqrt(1.0 + (dD_over_dY * dD_over_dY));
        const float I          = -sign(dY) * rho0 * H * (exp(-Ye / H) - exp(-Yf / H));

        return exp(-k * I);
    } else /* dY == 0.0 */ {

        const float rho_Ye = rho0 * exp(-Ye / H);

        return exp(-dD * rho_Ye);
    }

}


uniform sampler2D depth;

uniform vec3  fog_color;
uniform float z_near;
uniform float z_far;
uniform mat4  inv_projview;


float get_view_space_depth(float screen_depth) {
    // Linearization back from non-linear depth encoding:
    //   screen_depth = (1/z - 1/n) / (1/f - 1/n)
    // Where z, n and f are: view-space z, z_near and z_far respectively.
    return (z_near * z_far) /
        (z_far - screen_depth * (z_far - z_near));
}


void main() {

    float screen_depth = texture(depth, tex_coords).r;
    float view_depth   = get_view_space_depth(screen_depth);

    float w_clip    = view_depth;
    vec4  pos_ndc   = vec4(vec3(tex_coords, screen_depth) * 2.0 - 1.0, 1.0);
    vec4  pos_clip  = pos_ndc * w_clip;
    vec4  pos_world = inv_projview * pos_clip;


    float fog_factor = 1.0 - transmittance(pos_world.xyz);

    // And then just blend.
    frag_color = vec4(fog_color, fog_factor);

    // For debug, fog only:
    // frag_color = vec4(fog_color * vec3(fog_factor), 1.0);
}
