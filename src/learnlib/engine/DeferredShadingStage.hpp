#pragma once
#include "GLShaders.hpp"
#include "LightCasters.hpp"
#include "PostprocessRenderer.hpp"
#include "QuadRenderer.hpp"
#include "RenderStage.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "GBuffer.hpp"
#include <entt/entity/fwd.hpp>
#include <utility>


namespace learn {


class DeferredShadingStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/dfr_shading.vert")
            .load_frag("src/shaders/dfr_shading_simple.frag")
            .get()
    };

    SharedStorageView<GBuffer> gbuffer_;

    SSBOWithIntermediateBuffer<light::Point> plights_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    QuadRenderer quad_renderer_;

public:
    DeferredShadingStage(SharedStorageView<GBuffer> gbuffer)
        : gbuffer_{ std::move(gbuffer) }
    {}

    void operator()(const RenderEnginePrimaryInterface&,
        const entt::registry&);

private:
    void update_point_light_buffers(const entt::registry& registry);

};




} // namespace learn
