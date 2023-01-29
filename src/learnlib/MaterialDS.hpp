#pragma once
#include "MaterialTraits.hpp"
#include "GLObjects.hpp"
#include "Shared.hpp"

namespace learn {


struct MaterialDS {
    std::array<Shared<TextureHandle>, 2> textures;
    gl::GLfloat shininess{};
};

struct MaterialDSLocations {
    std::array<gl::GLint, 2> textures;
    gl::GLint shininess;
};

template<>
struct MaterialTraits<MaterialDS> {
    constexpr static std::array<MaterialParams, 2> texparams {{
        { "material.diffuse", TextureType::diffuse,
            gl::GL_TEXTURE_2D, gl::GL_TEXTURE0 },
        { "material.specular", TextureType::specular,
            gl::GL_TEXTURE_2D, gl::GL_TEXTURE1 }
    }};

    using locations_type = MaterialDSLocations;
};



template<>
void apply_material<MaterialDS>(ActiveShaderProgram& asp, const MaterialDS& mat,
    const MaterialDSLocations& locations);

template<>
void apply_material<MaterialDS>(ActiveShaderProgram& asp, const MaterialDS& mat);

template<>
typename MaterialTraits<MaterialDS>::locations_type query_locations<MaterialDS>(ShaderProgram& sp);



} // namespace learn
