#pragma once
#include "GBuffer.hpp"
#include "GLShaders.hpp"
#include "RenderStage.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include <entt/entity/fwd.hpp>



namespace learn {



class DeferredGeometryStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/non_instanced.vert")
            .load_frag("src/shaders/dfr_geometry_mat_ds.frag")
            .get()
    };

    SharedStorageMutableView<GBuffer> gbuffer_;

public:
    DeferredGeometryStage(SharedStorageMutableView<GBuffer> gbuffer_view)
        : gbuffer_{ std::move(gbuffer_view) }
    {}

    void operator()(const RenderEnginePrimaryInterface&, const entt::registry&);

};




} // namespace learn
