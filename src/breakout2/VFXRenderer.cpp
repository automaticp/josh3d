#include "VFXRenderer.hpp"
#include "ShaderBuilder.hpp"


VFXRenderer::VFXRenderer(gl::GLsizei width, gl::GLsizei height)
    : ppdb_{ width, height }
    , pp_shake_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_shake.vert")
            .load_frag("src/shaders/pp_kernel_blur.frag")
            .get()
    }
    , pp_chaos_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_chaos.vert")
            .load_frag("src/shaders/pp_kernel_edge.frag")
            .get()
    }
    , pp_confuse_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_confuse.vert")
            .load_frag("src/shaders/pp_invert.frag")
            .get()
    }
{}

