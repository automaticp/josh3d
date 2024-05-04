#ifndef UTILS_RANDOM_GLSL
#define UTILS_RANDOM_GLSL
// #version 330 core


// Needs a unique (per-invocation) seed.
// Some possible examples:
//     gl_VertexID                                        // Vertex Shader
//     uint(gl_FragCoord.y) << 16 | uint(gl_FragCoord.x); // Fragment Shader
//     gl_LocalInvocationIndex                            // Compute Shader
uint pcg32(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return result;
}


// Uniformly distributed floating point number from 0 to 1.
// Must be seeded from a uniform random bit. Example:
//     random_gamma(pcg32(random_id))
float random_gamma(uint urb) {
    return uintBitsToFloat(urb >> 9 | 0x3F800000) - 1.0;
}


#endif
